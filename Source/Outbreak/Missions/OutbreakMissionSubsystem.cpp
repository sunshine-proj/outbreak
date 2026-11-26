#include "Missions/OutbreakMissionSubsystem.h"

#include "Combat/OutbreakHealthComponent.h"
#include "Engine/World.h"
#include "Game/OutbreakGameState.h"
#include "GameFramework/Pawn.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "Missions/OutbreakObjective.h"
#include "Outbreak.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "OutbreakMission"

void UOutbreakMissionSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimer);
		World->GetTimerManager().ClearTimer(RestartTimer);
	}
	Super::Deinitialize();
}

bool UOutbreakMissionSubsystem::InitializeMission(const UOutbreakMissionDefinition* InDefinition)
{
	if (!InDefinition)
	{
		return false;
	}

	TArray<FText> Errors;
	if (!InDefinition->Validate(Errors))
	{
		for (const FText& Error : Errors)
		{
			UE_LOG(LogOutbreak, Error, TEXT("Mission %s is invalid: %s"), *InDefinition->GetName(), *Error.ToString());
		}
		return false;
	}

	// Definitions are immutable at runtime; the pointer is non-const only to be a UPROPERTY.
	Definition = const_cast<UOutbreakMissionDefinition*>(InDefinition);

	// Per-play copies, so objective state never writes into the asset.
	Objectives.Reset();
	for (const UOutbreakObjective* Template : InDefinition->Objectives)
	{
		Objectives.Add(DuplicateObject<UOutbreakObjective>(Template, this));
	}

	MissionState = EOutbreakMissionState::NotStarted;
	CurrentObjectiveIndex = INDEX_NONE;
	ResultText = FText::GetEmpty();
	PublishStatus();

	UE_LOG(LogOutbreak, Log, TEXT("Mission %s initialized with %d objectives."), *InDefinition->GetName(), Objectives.Num());
	return true;
}

void UOutbreakMissionSubsystem::StartMission()
{
	if (!Definition || Objectives.IsEmpty() || MissionState != EOutbreakMissionState::NotStarted)
	{
		return;
	}

	SetMissionState(EOutbreakMissionState::InProgress);
	ActivateObjective(0);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(UpdateTimer, this, &ThisClass::UpdateMission, Definition->UpdateInterval, /*bLoop=*/ true);
	}
	PublishStatus();
}

UOutbreakObjective* UOutbreakMissionSubsystem::GetCurrentObjective() const
{
	return Objectives.IsValidIndex(CurrentObjectiveIndex) ? Objectives[CurrentObjectiveIndex].Get() : nullptr;
}

void UOutbreakMissionSubsystem::ActivateObjective(int32 Index)
{
	CurrentObjectiveIndex = Index;
	UOutbreakObjective* Objective = GetCurrentObjective();
	if (!Objective)
	{
		return;
	}

	Objective->SetState(EOutbreakObjectiveState::Active);
	Objective->Activate(*this);
	UE_LOG(LogOutbreak, Log, TEXT("Objective %d/%d: %s"), Index + 1, Objectives.Num(), *Objective->GetDescription().ToString());
	OnObjectiveChanged.Broadcast(Index);
}

void UOutbreakMissionSubsystem::UpdateMission()
{
	if (MissionState != EOutbreakMissionState::InProgress)
	{
		return;
	}

	// The last standing teammate may have gone down while the player was waiting for a revive.
	CheckSquadFailure();
	if (MissionState != EOutbreakMissionState::InProgress)
	{
		return;
	}

	UOutbreakObjective* Objective = GetCurrentObjective();
	if (!Objective)
	{
		return;
	}

	switch (Objective->Evaluate(*this))
	{
	case EOutbreakObjectiveState::Completed:
		Objective->SetState(EOutbreakObjectiveState::Completed);
		Objective->Deactivate();
		if (CurrentObjectiveIndex + 1 < Objectives.Num())
		{
			ActivateObjective(CurrentObjectiveIndex + 1);
		}
		else
		{
			CompleteMission();
		}
		break;

	case EOutbreakObjectiveState::Failed:
		Objective->SetState(EOutbreakObjectiveState::Failed);
		FailMission(FText::Format(LOCTEXT("ObjectiveFailed", "Objective failed: {0}"), Objective->GetDescription()));
		break;

	default:
		break;
	}

	PublishStatus();
}

void UOutbreakMissionSubsystem::CompleteMission()
{
	EndMission(EOutbreakMissionState::Completed, LOCTEXT("Completed", "Mission complete"));
}

void UOutbreakMissionSubsystem::FailMission(const FText& Reason)
{
	EndMission(EOutbreakMissionState::Failed, Reason);
}

void UOutbreakMissionSubsystem::EndMission(EOutbreakMissionState EndState, const FText& Result)
{
	if (MissionState != EOutbreakMissionState::InProgress)
	{
		return;
	}

	if (UOutbreakObjective* Objective = GetCurrentObjective())
	{
		Objective->Deactivate();
	}

	ResultText = Result;
	SetMissionState(EndState);
	UE_LOG(LogOutbreak, Log, TEXT("Mission %s: %s"), *GetNameSafe(Definition), *Result.ToString());

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimer);
		if (Definition && Definition->RestartDelay > 0.0f)
		{
			World->GetTimerManager().SetTimer(RestartTimer, this, &ThisClass::RestartLevel, Definition->RestartDelay, false);
		}
	}
	PublishStatus();
}

void UOutbreakMissionSubsystem::RestartLevel()
{
	if (UWorld* World = GetWorld())
	{
		// Prototype flow: replay the level. The mission summary screen is Phase 11.
		World->ServerTravel(TEXT("?restart"));
	}
}

void UOutbreakMissionSubsystem::SetMissionState(EOutbreakMissionState NewState)
{
	if (MissionState != NewState)
	{
		MissionState = NewState;
		OnMissionStateChanged.Broadcast(NewState);
	}
}

// --- Players -------------------------------------------------------------------------------------

void UOutbreakMissionSubsystem::RegisterPlayer(APawn* PlayerPawn)
{
	if (!PlayerPawn)
	{
		return;
	}

	Players.RemoveAll([](const TWeakObjectPtr<APawn>& Entry) { return !Entry.IsValid(); });
	Players.AddUnique(PlayerPawn);

	if (UOutbreakHealthComponent* Health = PlayerPawn->FindComponentByClass<UOutbreakHealthComponent>())
	{
		Health->OnHealthStateChanged.AddUniqueDynamic(this, &ThisClass::HandlePlayerHealthStateChanged);
	}
}

TArray<APawn*> UOutbreakMissionSubsystem::GetLivingPlayerPawns() const
{
	TArray<APawn*> Result;
	for (const TWeakObjectPtr<APawn>& Entry : Players)
	{
		APawn* Pawn = Entry.Get();
		if (!IsValid(Pawn))
		{
			continue;
		}
		const UOutbreakHealthComponent* Health = Pawn->FindComponentByClass<UOutbreakHealthComponent>();
		if (!Health || Health->IsAlive())
		{
			Result.Add(Pawn);
		}
	}
	return Result;
}

void UOutbreakMissionSubsystem::HandlePlayerHealthStateChanged(UOutbreakHealthComponent* HealthComponent,
	EOutbreakHealthState NewState, EOutbreakHealthState OldState)
{
	if (NewState != EOutbreakHealthState::Alive)
	{
		CheckSquadFailure();
	}
}

void UOutbreakMissionSubsystem::CheckSquadFailure()
{
	if (MissionState != EOutbreakMissionState::InProgress || !Definition || !Definition->bFailOnPlayerDeath)
	{
		return;
	}

	// While any registered player is standing, the mission goes on.
	TArray<const APawn*> DownedPlayers;
	for (const TWeakObjectPtr<APawn>& Entry : Players)
	{
		const APawn* Pawn = Entry.Get();
		const UOutbreakHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UOutbreakHealthComponent>() : nullptr;
		if (!Health)
		{
			continue;
		}
		if (Health->IsAlive())
		{
			return;
		}
		if (Health->IsIncapacitated())
		{
			DownedPlayers.Add(Pawn);
		}
	}

	// Downed players can still be revived while a squadmate stands (Phase 10).
	for (const APawn* Pawn : DownedPlayers)
	{
		const UOutbreakSquadMemberComponent* Member = Pawn->FindComponentByClass<UOutbreakSquadMemberComponent>();
		const UOutbreakSquadComponent* Squad = Member ? Member->GetSquad() : nullptr;
		if (Squad && Squad->GetAvailableTeammates().Num() > 0)
		{
			return;
		}
	}

	if (!DownedPlayers.IsEmpty())
	{
		FailMission(LOCTEXT("SquadDown", "Your squad is down; nobody is left to revive you"));
		return;
	}
	FailMission(Players.Num() > 1
		? LOCTEXT("AllPlayersKilled", "All players were killed")
		: LOCTEXT("PlayerKilled", "You were killed"));
}

// --- Status --------------------------------------------------------------------------------------

FOutbreakMissionStatus UOutbreakMissionSubsystem::BuildStatus() const
{
	FOutbreakMissionStatus Status;
	Status.State = MissionState;
	Status.MissionName = Definition ? Definition->DisplayName : FText::GetEmpty();
	Status.ObjectiveIndex = FMath::Max(0, CurrentObjectiveIndex);
	Status.ObjectiveCount = Objectives.Num();
	Status.ResultText = ResultText;

	if (const UOutbreakObjective* Objective = GetCurrentObjective())
	{
		Status.ObjectiveDescription = Objective->GetDescription();
		Status.ObjectiveProgress = Objective->GetProgressText();
	}
	return Status;
}

void UOutbreakMissionSubsystem::PublishStatus() const
{
	const UWorld* World = GetWorld();
	AOutbreakGameState* GameState = World ? World->GetGameState<AOutbreakGameState>() : nullptr;
	if (GameState && GameState->HasAuthority())
	{
		GameState->SetMissionStatus(BuildStatus());
	}
}

#undef LOCTEXT_NAMESPACE
