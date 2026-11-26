#include "Game/OutbreakGameMode.h"

#include "Combat/OutbreakTeam.h"
#include "Engine/World.h"
#include "Environment/OutbreakTimeOfDay.h"
#include "Game/OutbreakGameState.h"
#include "Game/OutbreakPlayerState.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Outbreak.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Player/OutbreakPlayerController.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadSelection.h"
#include "Squad/OutbreakSquadSettings.h"

AOutbreakGameMode::AOutbreakGameMode()
{
	DefaultPawnClass = AOutbreakPlayerCharacter::StaticClass();
	PlayerControllerClass = AOutbreakPlayerController::StaticClass();
	PlayerStateClass = AOutbreakPlayerState::StaticClass();
	GameStateClass = AOutbreakGameState::StaticClass();
}

void AOutbreakGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// AI perception uses the global solver to tell enemies from friends.
	FGenericTeamId::SetAttitudeSolver(&OutbreakTeam::GetAttitude);

	if (MissionDefinition)
	{
		if (UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>())
		{
			Mission->InitializeMission(MissionDefinition);
		}
	}
}

void AOutbreakGameMode::InitGameState()
{
	Super::InitGameState();

	// Clients need the mission and the squad roster to show the briefing.
	if (AOutbreakGameState* OutbreakGameState = GetGameState<AOutbreakGameState>())
	{
		OutbreakGameState->SetMissionSetup(MissionDefinition, ResolveSquadConfiguration());
	}
}

UOutbreakSquadConfiguration* AOutbreakGameMode::ResolveSquadConfiguration() const
{
	if (MissionDefinition && MissionDefinition->SquadConfiguration)
	{
		return MissionDefinition->SquadConfiguration;
	}
	if (SquadConfiguration)
	{
		return SquadConfiguration;
	}
	const UOutbreakSquadSettings* Settings = GetDefault<UOutbreakSquadSettings>();
	return Settings->DefaultSquadConfiguration.IsNull() ? nullptr : Settings->DefaultSquadConfiguration.LoadSynchronous();
}

bool AOutbreakGameMode::ShouldRunBriefing() const
{
	return MissionDefinition && MissionDefinition->bShowBriefing
		&& !UGameplayStatics::HasOption(OptionsString, TEXT("SkipBriefing"));
}

void AOutbreakGameMode::StartPlay()
{
	Super::StartPlay();

	// Environment configuration: the mission's hour replaces the level's (after the clock's BeginPlay).
	if (MissionDefinition && MissionDefinition->StartHour >= 0.0f)
	{
		if (AOutbreakTimeOfDay* TimeOfDay = AOutbreakTimeOfDay::Find(GetWorld()))
		{
			TimeOfDay->SetHour(MissionDefinition->StartHour);
		}
	}

	UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>();
	if (!Mission || !Mission->GetMissionDefinition())
	{
		return;
	}

	if (!ShouldRunBriefing())
	{
		Mission->StartMission();
		return;
	}

	Mission->BeginBriefing();
	// Players spawned before StartPlay are waiting in the briefing.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Player = It->Get();
		if (Player && Player->GetPawn() && !Mission->IsPlayerDeployed(Player->GetPawn()))
		{
			PauseForBriefing(Player);
			break;
		}
	}
}

void AOutbreakGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (!NewPlayer || !NewPlayer->GetPawn())
	{
		return;
	}

	UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>();
	if (Mission)
	{
		Mission->RegisterPlayer(NewPlayer->GetPawn());
	}

	const AOutbreakPlayerState* PlayerState = NewPlayer->GetPlayerState<AOutbreakPlayerState>();
	const UOutbreakSquadComponent* Squad = PlayerState ? PlayerState->GetSquadComponent() : nullptr;
	if (!Squad || Squad->IsSquadInitialized())
	{
		return;
	}

	// A player joining before the mission is under way waits in the briefing; anyone later deploys at once.
	const EOutbreakMissionState MissionState = Mission ? Mission->GetMissionState() : EOutbreakMissionState::NotStarted;
	const bool bBeforeMission = MissionState == EOutbreakMissionState::NotStarted || MissionState == EOutbreakMissionState::Briefing;
	// An invalid mission never initializes and never briefs; its players deploy straight away.
	if (Mission && Mission->GetMissionDefinition() && ShouldRunBriefing() && bBeforeMission)
	{
		if (MissionState == EOutbreakMissionState::Briefing)
		{
			PauseForBriefing(Cast<APlayerController>(NewPlayer));
		}
		return;
	}
	DeployPlayer(NewPlayer, nullptr);
}

void AOutbreakGameMode::HandleDeployRequest(APlayerController* Player, const FOutbreakSquadSelection& Selection)
{
	const AOutbreakPlayerState* PlayerState = Player ? Player->GetPlayerState<AOutbreakPlayerState>() : nullptr;
	const UOutbreakSquadComponent* Squad = PlayerState ? PlayerState->GetSquadComponent() : nullptr;
	if (!Player || !Player->GetPawn() || !Squad || Squad->IsSquadInitialized())
	{
		return;
	}
	DeployPlayer(Player, &Selection);
}

void AOutbreakGameMode::DeployPlayer(AController* Player, const FOutbreakSquadSelection* Selection)
{
	APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	AOutbreakPlayerState* PlayerState = Player ? Player->GetPlayerState<AOutbreakPlayerState>() : nullptr;
	UOutbreakSquadComponent* Squad = PlayerState ? PlayerState->GetSquadComponent() : nullptr;
	if (!Pawn || !Squad)
	{
		return;
	}

	UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>();
	if (Mission)
	{
		// Before the squad is built, so teammates form up around the starting location.
		Mission->MovePawnToDeployment(Pawn);
	}

	const UOutbreakSquadConfiguration* Configuration = ResolveSquadConfiguration();
	if (Selection)
	{
		Squad->InitializeSquadWithSelection(Configuration, Pawn, *Selection);
	}
	else
	{
		const int32 SizeOverride = UGameplayStatics::GetIntOption(OptionsString, TEXT("SquadSize"), 0);
		Squad->InitializeSquad(Configuration, Pawn, SizeOverride);
	}
	// Equipment is chosen in the briefing only.
	Squad->CloseLoadoutSelection();

	if (Mission)
	{
		Mission->NotifyPlayerDeployed(Pawn);
		if (Mission->GetMissionState() != EOutbreakMissionState::Briefing)
		{
			ResumeAfterBriefing();
		}
	}
}

void AOutbreakGameMode::HandleRestartRequest(APlayerController* Player)
{
	if (UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>())
	{
		// Only after the end; the subsystem ignores the request otherwise.
		UE_LOG(LogOutbreak, Log, TEXT("%s asked to replay the mission."), *GetNameSafe(Player));
		Mission->RestartMission();
	}
}

void AOutbreakGameMode::PauseForBriefing(APlayerController* Player)
{
	if (bPausedForBriefing || !Player || !MissionDefinition || !MissionDefinition->bPauseDuringBriefing)
	{
		return;
	}
	bPausedForBriefing = SetPause(Player);
}

void AOutbreakGameMode::ResumeAfterBriefing()
{
	if (bPausedForBriefing)
	{
		bPausedForBriefing = false;
		ClearPause();
	}
}
