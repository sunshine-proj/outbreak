#include "Missions/OutbreakMissionSubsystem.h"

#include "Combat/OutbreakHealthComponent.h"
#include "Engine/World.h"
#include "Game/OutbreakGameState.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Missions/OutbreakMissionCheckpoint.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "Missions/OutbreakMissionMarker.h"
#include "Missions/OutbreakObjective.h"
#include "Missions/OutbreakObjectiveGraph.h"
#include "Missions/OutbreakObjectiveQueries.h"
#include "Outbreak.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "OutbreakMission"

void UOutbreakMissionSubsystem::Deinitialize()
{
	CleanupMission();
	if (UWorld* World = GetWorld())
	{
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

	// A previous mission in this world is cleaned up before the new one replaces it.
	CleanupMission();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RestartTimer);
	}

	// Definitions are immutable at runtime; the pointer is non-const only to be a UPROPERTY.
	Definition = const_cast<UOutbreakMissionDefinition*>(InDefinition);

	// Per-play copies, so objective state never writes into the asset.
	Objectives.Reset();
	TArray<FOutbreakObjectiveNode> Nodes;
	for (const UOutbreakObjective* Template : InDefinition->Objectives)
	{
		UOutbreakObjective* Objective = DuplicateObject<UOutbreakObjective>(Template, this);
		Objective->SetState(EOutbreakObjectiveState::Inactive);
		Objectives.Add(Objective);
		Nodes.Add(FOutbreakObjectiveNode::FromObjective(*Objective));
	}
	// Validated above, so this succeeds.
	OutbreakObjectiveGraph::Resolve(Nodes, ObjectivePrerequisites, Errors);

	Players.Reset();
	DeployedPlayers.Reset();
	ExtractedSoldiers.Reset();
	MissionState = EOutbreakMissionState::NotStarted;
	CurrentObjectiveIndex = INDEX_NONE;
	PendingCheckpointIndex = INDEX_NONE;
	MissionTime = 0.0f;
	ResultText = FText::GetEmpty();
	FinalResults = FOutbreakMissionResults();
	PublishStatus();

	UE_LOG(LogOutbreak, Log, TEXT("Mission %s initialized with %d objectives."), *InDefinition->GetName(), Objectives.Num());
	return true;
}

void UOutbreakMissionSubsystem::BeginBriefing()
{
	if (Definition && MissionState == EOutbreakMissionState::NotStarted)
	{
		SetMissionState(EOutbreakMissionState::Briefing);
		PublishStatus();
	}
}

void UOutbreakMissionSubsystem::StartMission()
{
	if (!Definition || Objectives.IsEmpty()
		|| (MissionState != EOutbreakMissionState::NotStarted && MissionState != EOutbreakMissionState::Briefing))
	{
		return;
	}

	MissionTime = 0.0f;
	BeginEvaluation();
}

bool UOutbreakMissionSubsystem::StartMissionFromCheckpoint(const FOutbreakMissionCheckpoint& Checkpoint)
{
	if (!Definition || Objectives.IsEmpty()
		|| (MissionState != EOutbreakMissionState::NotStarted && MissionState != EOutbreakMissionState::Briefing))
	{
		return false;
	}

	// The caller (the campaign) makes sure the checkpoint is this mission's; here it must fit the objectives.
	TArray<FText> Errors;
	if (!CanResumeFrom(Checkpoint, Errors))
	{
		for (const FText& Error : Errors)
		{
			UE_LOG(LogOutbreak, Warning, TEXT("Mission %s: checkpoint not used: %s"), *GetNameSafe(Definition), *Error.ToString());
		}
		StartMission();
		return false;
	}

	// Finished objectives keep their state without running or broadcasting, so encounters and
	// equipment tied to them do not fire again.
	for (int32 Index = 0; Index < Objectives.Num(); ++Index)
	{
		Objectives[Index]->SetState(OutbreakMissionCheckpointRules::ToSavedState(Checkpoint.ObjectiveStates[Index]));
	}
	MissionTime = Checkpoint.MissionTime;
	UE_LOG(LogOutbreak, Log, TEXT("Mission %s resumed from a checkpoint at %.0f s."), *GetNameSafe(Definition), MissionTime);
	BeginEvaluation();
	return true;
}

bool UOutbreakMissionSubsystem::CanResumeFrom(const FOutbreakMissionCheckpoint& Checkpoint, TArray<FText>& OutErrors) const
{
	if (!Definition)
	{
		return false;
	}
	TArray<FOutbreakObjectiveNode> Nodes;
	for (const UOutbreakObjective* Objective : Objectives)
	{
		Nodes.Add(FOutbreakObjectiveNode::FromObjective(*Objective));
	}
	return OutbreakMissionCheckpointRules::Validate(Checkpoint, Nodes, OutErrors);
}

void UOutbreakMissionSubsystem::BeginEvaluation()
{
	PendingCheckpointIndex = INDEX_NONE;
	SetMissionState(EOutbreakMissionState::InProgress);
	ActivateReadyObjectives();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(UpdateTimer, this, &ThisClass::HandleUpdateTimer, Definition->UpdateInterval, /*bLoop=*/ true);
	}
	PublishStatus();
}

FOutbreakMissionCheckpoint UOutbreakMissionSubsystem::CaptureCheckpoint(int32 CompletedObjectiveIndex) const
{
	FOutbreakMissionCheckpoint Checkpoint;
	if (!Definition)
	{
		return Checkpoint;
	}
	Checkpoint.MissionId = Definition->GetResolvedMissionId();
	Checkpoint.MissionTime = MissionTime;
	for (const UOutbreakObjective* Objective : Objectives)
	{
		const EOutbreakObjectiveState State = Objective ? Objective->GetState() : EOutbreakObjectiveState::Inactive;
		Checkpoint.ObjectiveStates.Add(OutbreakMissionCheckpointRules::ToSavedState(State));
		Checkpoint.CompletedObjectiveCount += State == EOutbreakObjectiveState::Completed ? 1 : 0;
	}
	const UOutbreakObjective* Completed = GetObjective(CompletedObjectiveIndex);
	Checkpoint.ResumeMarkerId = Completed && !Completed->CheckpointMarkerId.IsNone() ? Completed->CheckpointMarkerId : Definition->DeploymentMarkerId;
	return Checkpoint;
}

UOutbreakObjective* UOutbreakMissionSubsystem::GetCurrentObjective() const
{
	return GetObjective(CurrentObjectiveIndex);
}

UOutbreakObjective* UOutbreakMissionSubsystem::GetObjective(int32 Index) const
{
	return Objectives.IsValidIndex(Index) ? Objectives[Index].Get() : nullptr;
}

UOutbreakObjective* UOutbreakMissionSubsystem::FindObjectiveById(FName ObjectiveId) const
{
	if (ObjectiveId.IsNone())
	{
		return nullptr;
	}
	for (UOutbreakObjective* Objective : Objectives)
	{
		if (Objective && Objective->GetObjectiveId() == ObjectiveId)
		{
			return Objective;
		}
	}
	return nullptr;
}

TArray<EOutbreakObjectiveState> UOutbreakMissionSubsystem::GetObjectiveStates() const
{
	TArray<EOutbreakObjectiveState> States;
	States.Reserve(Objectives.Num());
	for (const UOutbreakObjective* Objective : Objectives)
	{
		States.Add(Objective ? Objective->GetState() : EOutbreakObjectiveState::Failed);
	}
	return States;
}

void UOutbreakMissionSubsystem::ActivateObjective(int32 Index)
{
	UOutbreakObjective* Objective = GetObjective(Index);
	if (!Objective)
	{
		return;
	}

	Objective->SetState(EOutbreakObjectiveState::Active);
	Objective->SetActivationTime(MissionTime);
	Objective->Activate(*this);
	UE_LOG(LogOutbreak, Log, TEXT("Objective %d/%d%s: %s"), Index + 1, Objectives.Num(),
		Objective->IsOptional() ? TEXT(" (optional)") : TEXT(""), *Objective->GetDescription().ToString());
	OnObjectiveChanged.Broadcast(Index);
	OnObjectiveStateChanged.Broadcast(Index, EOutbreakObjectiveState::Active);
}

void UOutbreakMissionSubsystem::FinishObjective(int32 Index, EOutbreakObjectiveState EndState)
{
	UOutbreakObjective* Objective = GetObjective(Index);
	if (!Objective)
	{
		return;
	}

	const bool bWasActive = Objective->GetState() == EOutbreakObjectiveState::Active;
	Objective->SetState(EndState);
	if (bWasActive)
	{
		Objective->Deactivate();
	}
	UE_LOG(LogOutbreak, Log, TEXT("Objective %d/%d %s: %s"), Index + 1, Objectives.Num(),
		EndState == EOutbreakObjectiveState::Completed ? TEXT("completed") : TEXT("failed"), *Objective->GetDescription().ToString());
	if (EndState == EOutbreakObjectiveState::Completed && !Objective->IsOptional())
	{
		PendingCheckpointIndex = Index;
	}
	OnObjectiveStateChanged.Broadcast(Index, EndState);
}

void UOutbreakMissionSubsystem::ActivateReadyObjectives()
{
	const TArray<EOutbreakObjectiveState> States = GetObjectiveStates();
	for (int32 Index = 0; Index < Objectives.Num(); ++Index)
	{
		if (States[Index] != EOutbreakObjectiveState::Inactive)
		{
			continue;
		}
		if (OutbreakObjectiveGraph::IsBlocked(Index, ObjectivePrerequisites, States))
		{
			// Only optional objectives can be blocked (validation); they can no longer be achieved.
			FinishObjective(Index, EOutbreakObjectiveState::Failed);
		}
		else if (OutbreakObjectiveGraph::ArePrerequisitesMet(Index, ObjectivePrerequisites, States))
		{
			ActivateObjective(Index);
		}
	}
	UpdateCurrentObjectiveIndex();
}

void UOutbreakMissionSubsystem::UpdateCurrentObjectiveIndex()
{
	int32 FirstActiveOptional = INDEX_NONE;
	for (int32 Index = 0; Index < Objectives.Num(); ++Index)
	{
		const UOutbreakObjective* Objective = Objectives[Index];
		if (!Objective || Objective->GetState() != EOutbreakObjectiveState::Active)
		{
			continue;
		}
		if (!Objective->IsOptional())
		{
			CurrentObjectiveIndex = Index;
			return;
		}
		if (FirstActiveOptional == INDEX_NONE)
		{
			FirstActiveOptional = Index;
		}
	}
	if (FirstActiveOptional != INDEX_NONE)
	{
		CurrentObjectiveIndex = FirstActiveOptional;
	}
	// Nothing active: keep the last current objective so the final status still names it.
}

bool UOutbreakMissionSubsystem::AreRequiredObjectivesComplete() const
{
	for (const UOutbreakObjective* Objective : Objectives)
	{
		if (Objective && !Objective->IsOptional() && Objective->GetState() != EOutbreakObjectiveState::Completed)
		{
			return false;
		}
	}
	return true;
}

void UOutbreakMissionSubsystem::HandleUpdateTimer()
{
	AdvanceMission(Definition ? Definition->UpdateInterval : 0.0f);
}

void UOutbreakMissionSubsystem::UpdateMission()
{
	AdvanceMission(0.0f);
}

void UOutbreakMissionSubsystem::AdvanceMission(float DeltaSeconds)
{
	if (MissionState != EOutbreakMissionState::InProgress)
	{
		return;
	}
	MissionTime += FMath::Max(0.0f, DeltaSeconds);

	// The last standing teammate may have gone down while the player was waiting for a revive.
	CheckSquadFailure();
	if (MissionState != EOutbreakMissionState::InProgress)
	{
		return;
	}

	// Evaluate every objective that was active at the start of this update. Objectives activated by
	// this update are evaluated next time, as in Phase 6.
	TArray<int32> ActiveIndices;
	for (int32 Index = 0; Index < Objectives.Num(); ++Index)
	{
		if (Objectives[Index] && Objectives[Index]->GetState() == EOutbreakObjectiveState::Active)
		{
			ActiveIndices.Add(Index);
		}
	}

	for (const int32 Index : ActiveIndices)
	{
		UOutbreakObjective* Objective = Objectives[Index];
		// Phase 14 time limits run out before the objective gets another evaluation.
		const EOutbreakObjectiveState Result = Objective->HasTimedOut(MissionTime)
			? EOutbreakObjectiveState::Failed : Objective->Evaluate(*this);
		if (Result == EOutbreakObjectiveState::Completed)
		{
			FinishObjective(Index, EOutbreakObjectiveState::Completed);
		}
		else if (Result == EOutbreakObjectiveState::Failed)
		{
			FinishObjective(Index, EOutbreakObjectiveState::Failed);
			if (!Objective->IsOptional())
			{
				FailMission(Objective->HasTimedOut(MissionTime)
					? FText::Format(LOCTEXT("ObjectiveTimedOut", "Out of time: {0}"), Objective->GetDescription())
					: FText::Format(LOCTEXT("ObjectiveFailed", "Objective failed: {0}"), Objective->GetDescription()));
				return;
			}
		}
		if (MissionState != EOutbreakMissionState::InProgress)
		{
			return;
		}
	}

	if (AreRequiredObjectivesComplete())
	{
		CompleteMission();
		return;
	}

	ActivateReadyObjectives();

	const bool bAnyActive = Objectives.ContainsByPredicate([](const UOutbreakObjective* Objective)
	{
		return Objective && Objective->GetState() == EOutbreakObjectiveState::Active;
	});
	if (!bAnyActive)
	{
		// Validation rules this out; fail visibly rather than soft-lock.
		UE_LOG(LogOutbreak, Error, TEXT("Mission %s: no objective is active and required objectives remain."), *GetNameSafe(Definition));
		FailMission(LOCTEXT("Stalled", "The mission's objectives cannot progress"));
		return;
	}

	PublishStatus();

	// After the newly unlocked objectives are active, so a checkpoint resumes with them.
	if (PendingCheckpointIndex != INDEX_NONE)
	{
		const int32 CheckpointIndex = PendingCheckpointIndex;
		PendingCheckpointIndex = INDEX_NONE;
		if (Definition && Definition->bCheckpointsEnabled)
		{
			OnCheckpointReached.Broadcast(CheckpointIndex);
		}
	}
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

	ResultText = Result;
	CleanupMission();
	SetMissionState(EndState);
	FinalResults = BuildResults();
	UE_LOG(LogOutbreak, Log, TEXT("Mission %s: %s (%.0f s, required %d/%d, optional %d/%d)"), *GetNameSafe(Definition), *Result.ToString(),
		FinalResults.Duration, FinalResults.RequiredObjectivesCompleted, FinalResults.RequiredObjectiveCount,
		FinalResults.OptionalObjectivesCompleted, FinalResults.OptionalObjectiveCount);

	const UWorld* World = GetWorld();
	if (AOutbreakGameState* GameState = World ? World->GetGameState<AOutbreakGameState>() : nullptr)
	{
		if (GameState->HasAuthority())
		{
			// Before the status, so clients reacting to the end state already have the results.
			GameState->SetMissionResults(FinalResults);
		}
	}
	OnMissionEnded.Broadcast(FinalResults);

	if (World && Definition && Definition->RestartDelay > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(RestartTimer, this, &ThisClass::RestartMission, Definition->RestartDelay, false);
	}
	PublishStatus();
}

void UOutbreakMissionSubsystem::CleanupMission()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimer);
	}

	// Active objectives release their level actors; their states stay for the summary.
	for (UOutbreakObjective* Objective : Objectives)
	{
		if (Objective && Objective->GetState() == EOutbreakObjectiveState::Active)
		{
			Objective->Deactivate();
		}
	}

	for (const TWeakObjectPtr<APawn>& Entry : Players)
	{
		const APawn* Pawn = Entry.Get();
		if (UOutbreakHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UOutbreakHealthComponent>() : nullptr)
		{
			Health->OnHealthStateChanged.RemoveDynamic(this, &ThisClass::HandlePlayerHealthStateChanged);
		}
	}
}

void UOutbreakMissionSubsystem::RestartMission()
{
	if (MissionState != EOutbreakMissionState::Completed && MissionState != EOutbreakMissionState::Failed)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RestartTimer);
		// Replay the level. In a campaign the campaign subsystem keeps the launched mission (and
		// whether to resume its checkpoint) across the travel.
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

bool UOutbreakMissionSubsystem::MovePawnToDeployment(APawn* PlayerPawn, FName MarkerIdOverride) const
{
	const FName MarkerId = !MarkerIdOverride.IsNone() ? MarkerIdOverride : (Definition ? Definition->DeploymentMarkerId : NAME_None);
	if (!PlayerPawn || !Definition || MarkerId.IsNone())
	{
		return false;
	}

	const AOutbreakMissionMarker* Marker = AOutbreakMissionMarker::FindMarker(GetWorld(), MarkerId);
	if (!Marker)
	{
		UE_LOG(LogOutbreak, Warning, TEXT("Mission %s: no deployment marker '%s'; players deploy at their Player Start."),
			*GetNameSafe(Definition), *MarkerId.ToString());
		return false;
	}

	// Co-op: players deploying one after another stand side by side, alternating right and left.
	int32 Slot = 0;
	for (const TWeakObjectPtr<APawn>& Entry : DeployedPlayers)
	{
		Slot += Entry.IsValid() && Entry.Get() != PlayerPawn ? 1 : 0;
	}
	const float Side = Slot == 0 ? 0.0f : ((Slot % 2 == 1) ? 1.0f : -1.0f) * ((Slot + 1) / 2);
	const FRotator Rotation(0.0f, Marker->GetActorRotation().Yaw, 0.0f);
	const FVector Location = Marker->GetActorLocation() + Rotation.RotateVector(FVector::RightVector) * (Side * Definition->DeploymentSpacing);

	PlayerPawn->TeleportTo(Location, Rotation, /*bIsATest=*/ false, /*bNoCheck=*/ false);
	if (AController* Controller = PlayerPawn->GetController())
	{
		Controller->SetControlRotation(Rotation);
	}
	return true;
}

void UOutbreakMissionSubsystem::NotifyPlayerDeployed(APawn* PlayerPawn)
{
	if (!PlayerPawn)
	{
		return;
	}
	RegisterPlayer(PlayerPawn);
	DeployedPlayers.RemoveAll([](const TWeakObjectPtr<APawn>& Entry) { return !Entry.IsValid(); });
	DeployedPlayers.AddUnique(PlayerPawn);

	if (MissionState == EOutbreakMissionState::Briefing && AreAllPlayersDeployed())
	{
		StartMission();
	}
}

bool UOutbreakMissionSubsystem::IsPlayerDeployed(const APawn* PlayerPawn) const
{
	return PlayerPawn && DeployedPlayers.Contains(PlayerPawn);
}

bool UOutbreakMissionSubsystem::AreAllPlayersDeployed() const
{
	bool bAnyPlayer = false;
	for (const TWeakObjectPtr<APawn>& Entry : Players)
	{
		if (const APawn* Pawn = Entry.Get())
		{
			bAnyPlayer = true;
			if (!DeployedPlayers.Contains(Pawn))
			{
				return false;
			}
		}
	}
	return bAnyPlayer;
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

TArray<AActor*> UOutbreakMissionSubsystem::GetSquadSoldiers() const
{
	TArray<AActor*> Result;
	for (const TWeakObjectPtr<APawn>& Entry : Players)
	{
		if (APawn* Pawn = Entry.Get())
		{
			Result.AddUnique(Pawn);
		}
	}
	for (const TWeakObjectPtr<APawn>& Entry : Players)
	{
		const APawn* Pawn = Entry.Get();
		const UOutbreakSquadMemberComponent* Member = Pawn ? Pawn->FindComponentByClass<UOutbreakSquadMemberComponent>() : nullptr;
		const UOutbreakSquadComponent* Squad = Member ? Member->GetSquad() : nullptr;
		if (!Squad)
		{
			continue;
		}
		for (const UOutbreakSquadMemberComponent* SquadMember : Squad->GetMembers())
		{
			if (AActor* Owner = SquadMember ? SquadMember->GetOwner() : nullptr)
			{
				Result.AddUnique(Owner);
			}
		}
	}
	return Result;
}

void UOutbreakMissionSubsystem::RecordExtracted(const TArray<AActor*>& Soldiers)
{
	for (AActor* Soldier : Soldiers)
	{
		if (Soldier)
		{
			ExtractedSoldiers.AddUnique(Soldier);
		}
	}
}

bool UOutbreakMissionSubsystem::WasExtracted(const AActor* Soldier) const
{
	return Soldier && ExtractedSoldiers.Contains(Soldier);
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

	// Progress with the time left on a time-limited objective (Phase 14), e.g. "2 / 5, 1:30".
	auto BuildProgress = [this](const UOutbreakObjective& Objective)
	{
		const FText Progress = Objective.GetProgressText();
		const float Remaining = Objective.GetTimeRemaining(MissionTime);
		if (Remaining < 0.0f || Objective.GetState() != EOutbreakObjectiveState::Active)
		{
			return Progress;
		}
		const FText Time = OutbreakObjectiveQueries::FormatTime(Remaining);
		return Progress.IsEmpty() ? Time : FText::Format(LOCTEXT("ProgressWithTime", "{0}, {1}"), Progress, Time);
	};

	if (const UOutbreakObjective* Objective = GetCurrentObjective())
	{
		Status.ObjectiveDescription = Objective->GetDescription();
		Status.ObjectiveProgress = BuildProgress(*Objective);
	}

	Status.Objectives.Reserve(Objectives.Num());
	for (const UOutbreakObjective* Objective : Objectives)
	{
		FOutbreakObjectiveStatus& Entry = Status.Objectives.AddDefaulted_GetRef();
		if (Objective)
		{
			Entry.Description = Objective->GetDescription();
			Entry.State = Objective->GetState();
			Entry.bOptional = Objective->IsOptional();
			Entry.bHidden = Objective->IsHidden();
			if (Entry.State == EOutbreakObjectiveState::Active)
			{
				Entry.Progress = BuildProgress(*Objective);
			}
		}
	}
	return Status;
}

FOutbreakMissionResults UOutbreakMissionSubsystem::BuildResults() const
{
	FOutbreakMissionResults Results;
	if (Definition)
	{
		Results.MissionId = Definition->GetResolvedMissionId();
		Results.MissionName = Definition->DisplayName;
	}
	const bool bEnded = MissionState == EOutbreakMissionState::Completed || MissionState == EOutbreakMissionState::Failed;
	Results.Outcome = bEnded ? MissionState : EOutbreakMissionState::NotStarted;
	Results.ResultText = ResultText;
	Results.Duration = MissionTime;
	Results.Objectives = BuildStatus().Objectives;

	for (const FOutbreakObjectiveStatus& Objective : Results.Objectives)
	{
		const bool bCompleted = Objective.State == EOutbreakObjectiveState::Completed;
		if (Objective.bOptional)
		{
			++Results.OptionalObjectiveCount;
			Results.OptionalObjectivesCompleted += bCompleted ? 1 : 0;
		}
		else
		{
			++Results.RequiredObjectiveCount;
			Results.RequiredObjectivesCompleted += bCompleted ? 1 : 0;
		}
	}

	for (const AActor* Soldier : GetSquadSoldiers())
	{
		FOutbreakMissionMemberReport& Report = Results.Squad.AddDefaulted_GetRef();
		Report.bExtracted = WasExtracted(Soldier);
		if (const UOutbreakSquadMemberComponent* Member = Soldier->FindComponentByClass<UOutbreakSquadMemberComponent>())
		{
			Report.DisplayName = Member->GetDisplayName();
			Report.bIsPlayer = Member->IsPlayerMember() || Players.Contains(Soldier);
			Report.Status = Member->GetStatus();
		}
		else
		{
			Report.bIsPlayer = true;
			const UOutbreakHealthComponent* Health = Soldier->FindComponentByClass<UOutbreakHealthComponent>();
			Report.Status = !Health || Health->IsAlive() ? EOutbreakSquadMemberStatus::Active
				: (Health->IsIncapacitated() ? EOutbreakSquadMemberStatus::Incapacitated : EOutbreakSquadMemberStatus::Dead);
		}
		if (Report.DisplayName.IsEmpty())
		{
			const APawn* Pawn = Cast<APawn>(Soldier);
			const APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState() : nullptr;
			Report.DisplayName = PlayerState ? FText::FromString(PlayerState->GetPlayerName()) : LOCTEXT("PlayerName", "You");
		}
	}
	return Results;
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
