#include "Game/OutbreakGameMode.h"

#include "Campaign/OutbreakCampaignSubsystem.h"
#include "Combat/OutbreakTeam.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Environment/OutbreakTimeOfDay.h"
#include "Missions/OutbreakAlarmSubsystem.h"
#include "Game/OutbreakGameState.h"
#include "Game/OutbreakPlayerState.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Outbreak.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Player/OutbreakPlayerController.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadSelection.h"
#include "Squad/OutbreakSquadSettings.h"
#include "Squad/OutbreakSquadSnapshot.h"

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

	// A campaign launch on this level replaces the level's own mission (Phase 14).
	ActiveMission = MissionDefinition;
	if (UOutbreakCampaignSubsystem* Campaign = GetCampaign())
	{
		if (UOutbreakMissionDefinition* Launched = Campaign->GetLaunchedMission(MapName))
		{
			ActiveMission = Launched;
			bCampaignMission = true;
			if (const FOutbreakMissionCheckpoint* Checkpoint = Campaign->GetCheckpointToResume())
			{
				ResumeCheckpoint = *Checkpoint;
			}
			UE_LOG(LogOutbreak, Log, TEXT("Campaign mission %s%s."), *Campaign->GetLaunchedMissionId().ToString(),
				ResumeCheckpoint.IsSet() ? TEXT(", resuming from its checkpoint") : TEXT(""));
		}
	}

	if (ActiveMission)
	{
		if (UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>())
		{
			if (Mission->InitializeMission(ActiveMission))
			{
				Mission->OnCheckpointReached.AddUniqueDynamic(this, &ThisClass::HandleCheckpointReached);
				Mission->OnMissionEnded.AddUniqueDynamic(this, &ThisClass::HandleMissionEnded);

				// Checked before anyone deploys: an unusable checkpoint means the normal briefing flow.
				TArray<FText> Errors;
				if (ResumeCheckpoint.IsSet() && !Mission->CanResumeFrom(ResumeCheckpoint, Errors))
				{
					for (const FText& Error : Errors)
					{
						UE_LOG(LogOutbreak, Warning, TEXT("Checkpoint not used, the mission starts from its briefing: %s"), *Error.ToString());
					}
					ResumeCheckpoint = FOutbreakMissionCheckpoint();
				}
			}
			else
			{
				ResumeCheckpoint = FOutbreakMissionCheckpoint();
			}
		}
	}
}

UOutbreakCampaignSubsystem* AOutbreakGameMode::GetCampaign() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UOutbreakCampaignSubsystem>() : nullptr;
}

void AOutbreakGameMode::InitGameState()
{
	Super::InitGameState();

	// Clients need the mission and the squad roster to show the briefing.
	if (AOutbreakGameState* OutbreakGameState = GetGameState<AOutbreakGameState>())
	{
		OutbreakGameState->SetMissionSetup(ActiveMission, ResolveSquadConfiguration());
		if (bCampaignMission)
		{
			if (const UOutbreakCampaignSubsystem* Campaign = GetCampaign())
			{
				OutbreakGameState->SetCampaignStatus(Campaign->BuildStatus());
			}
		}
	}
}

UOutbreakSquadConfiguration* AOutbreakGameMode::ResolveSquadConfiguration() const
{
	if (ActiveMission && ActiveMission->SquadConfiguration)
	{
		return ActiveMission->SquadConfiguration;
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
	// A checkpoint resumes after deployment, with the squad it saved.
	return ActiveMission && ActiveMission->bShowBriefing && !ResumeCheckpoint.IsSet()
		&& !UGameplayStatics::HasOption(OptionsString, TEXT("SkipBriefing"));
}

void AOutbreakGameMode::StartPlay()
{
	Super::StartPlay();

	// Environment configuration: the mission's hour replaces the level's (after the clock's BeginPlay).
	if (ActiveMission && ActiveMission->StartHour >= 0.0f)
	{
		if (AOutbreakTimeOfDay* TimeOfDay = AOutbreakTimeOfDay::Find(GetWorld()))
		{
			TimeOfDay->SetHour(ActiveMission->StartHour);
		}
	}

	UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>();
	if (!Mission || !Mission->GetMissionDefinition())
	{
		return;
	}

	if (ResumeCheckpoint.IsSet())
	{
		StartFromCheckpoint();
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

void AOutbreakGameMode::HandleDeployRequest(APlayerController* Player, const FOutbreakDeploymentSelection& Selection)
{
	const AOutbreakPlayerState* PlayerState = Player ? Player->GetPlayerState<AOutbreakPlayerState>() : nullptr;
	const UOutbreakSquadComponent* Squad = PlayerState ? PlayerState->GetSquadComponent() : nullptr;
	if (!Player || !Player->GetPawn() || !Squad || Squad->IsSquadInitialized())
	{
		return;
	}
	DeployPlayer(Player, &Selection);
}

void AOutbreakGameMode::DeployPlayer(AController* Player, const FOutbreakDeploymentSelection* Selection)
{
	APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	AOutbreakPlayerState* PlayerState = Player ? Player->GetPlayerState<AOutbreakPlayerState>() : nullptr;
	UOutbreakSquadComponent* Squad = PlayerState ? PlayerState->GetSquadComponent() : nullptr;
	if (!Pawn || !Squad)
	{
		return;
	}

	// The first player to deploy is the host whose squad checkpoints save (single-player: the player).
	const bool bHost = !HostPlayer.IsValid();
	if (bHost)
	{
		HostPlayer = Player;
	}
	const UOutbreakSquadConfiguration* Configuration = ResolveSquadConfiguration();

	// A checkpoint rebuilds the host's saved squad; other players deploy as usual.
	FOutbreakDeploymentSelection SavedSelection;
	if (bHost && ResumeCheckpoint.IsSet() && !ResumeCheckpoint.Squad.IsEmpty() && !Selection)
	{
		SavedSelection = OutbreakSquadSnapshot::ToSelection(ResumeCheckpoint.Squad);
		TArray<FText> Errors;
		if (OutbreakSquadSelection::Validate(Configuration, OutbreakSquadSelection::ResolveLimits(Configuration), SavedSelection, Errors))
		{
			Selection = &SavedSelection;
		}
		else
		{
			// The configuration changed since the save (roster, loadouts or limits).
			for (const FText& Error : Errors)
			{
				UE_LOG(LogOutbreak, Warning, TEXT("Checkpoint squad no longer fits the configuration (%s); deploying the default squad."), *Error.ToString());
			}
			ResumeCheckpoint.Squad = FOutbreakSquadSnapshot();
		}
	}

	UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>();
	if (Mission)
	{
		// Before the squad is built, so teammates form up around the starting location.
		Mission->MovePawnToDeployment(Pawn, ResumeCheckpoint.IsSet() ? ResumeCheckpoint.ResumeMarkerId : NAME_None);
	}

	const int32 SizeOverride = UGameplayStatics::GetIntOption(OptionsString, TEXT("SquadSize"), 0);
	if (Selection)
	{
		Squad->InitializeSquadWithSelection(Configuration, Pawn, *Selection);
	}
	else
	{
		Squad->InitializeSquad(Configuration, Pawn, SizeOverride);
	}
	if (bHost)
	{
		HostSelection = Selection ? *Selection
			: OutbreakSquadSelection::MakeDefault(Configuration, OutbreakSquadSelection::ResolveLimits(Configuration), SizeOverride);
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

void AOutbreakGameMode::HandleRestartRequest(APlayerController* Player, bool bFromCheckpoint)
{
	UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>();
	const EOutbreakMissionState State = Mission ? Mission->GetMissionState() : EOutbreakMissionState::NotStarted;
	if (State != EOutbreakMissionState::Completed && State != EOutbreakMissionState::Failed)
	{
		// Only after the end.
		return;
	}
	UE_LOG(LogOutbreak, Log, TEXT("%s asked to replay the mission%s."), *GetNameSafe(Player), bFromCheckpoint ? TEXT(" from its checkpoint") : TEXT(""));
	if (bCampaignMission)
	{
		if (UOutbreakCampaignSubsystem* Campaign = GetCampaign())
		{
			Campaign->SetResumeOnRestart(bFromCheckpoint);
		}
	}
	Mission->RestartMission();
}

void AOutbreakGameMode::HandleContinueRequest(APlayerController* Player)
{
	const UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>();
	UOutbreakCampaignSubsystem* Campaign = GetCampaign();
	if (!bCampaignMission || !Campaign || !Mission || Mission->GetMissionState() != EOutbreakMissionState::Completed)
	{
		return;
	}
	UE_LOG(LogOutbreak, Log, TEXT("%s continues the campaign."), *GetNameSafe(Player));
	if (!Campaign->ContinueCampaign())
	{
		// Nothing left to play: the campaign screen shows the completed campaign.
		Campaign->ReturnToFrontEnd();
	}
}

void AOutbreakGameMode::HandleMainMenuRequest(APlayerController* Player)
{
	UOutbreakCampaignSubsystem* Campaign = GetCampaign();
	if (bCampaignMission && Campaign)
	{
		UE_LOG(LogOutbreak, Log, TEXT("%s returns to the campaign screen."), *GetNameSafe(Player));
		Campaign->ReturnToFrontEnd();
		return;
	}
	UKismetSystemLibrary::QuitGame(this, Player, EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/ false);
}

void AOutbreakGameMode::StartFromCheckpoint()
{
	UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>();
	if (!Mission)
	{
		return;
	}

	// After every actor's BeginPlay, so default weapons are in place before the saved ones replace them.
	const AController* Host = HostPlayer.Get();
	const AOutbreakPlayerState* PlayerState = Host ? Host->GetPlayerState<AOutbreakPlayerState>() : nullptr;
	UOutbreakSquadComponent* Squad = PlayerState ? PlayerState->GetSquadComponent() : nullptr;
	if (Squad && !ResumeCheckpoint.Squad.IsEmpty())
	{
		OutbreakSquadSnapshot::ApplySquad(*Squad, ResumeCheckpoint.Squad);
	}

	if (Mission->StartMissionFromCheckpoint(ResumeCheckpoint) && ResumeCheckpoint.bAlarmRaised)
	{
		if (UOutbreakAlarmSubsystem* Alarm = GetWorld()->GetSubsystem<UOutbreakAlarmSubsystem>())
		{
			Alarm->RaiseAlarm(ResumeCheckpoint.AlarmLocation, nullptr);
		}
	}
}

void AOutbreakGameMode::HandleCheckpointReached(int32 CompletedObjectiveIndex)
{
	UOutbreakCampaignSubsystem* Campaign = GetCampaign();
	const UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>();
	if (!bCampaignMission || !Campaign || !Mission)
	{
		return;
	}

	FOutbreakMissionCheckpoint Checkpoint = Mission->CaptureCheckpoint(CompletedObjectiveIndex);
	if (const UOutbreakAlarmSubsystem* Alarm = GetWorld()->GetSubsystem<UOutbreakAlarmSubsystem>())
	{
		Checkpoint.bAlarmRaised = Alarm->IsAlarmRaised();
		if (const AOutbreakGameState* OutbreakGameState = GetGameState<AOutbreakGameState>())
		{
			Checkpoint.AlarmLocation = OutbreakGameState->GetAlarmLocation();
		}
	}
	const AController* Host = HostPlayer.Get();
	const AOutbreakPlayerState* PlayerState = Host ? Host->GetPlayerState<AOutbreakPlayerState>() : nullptr;
	if (const UOutbreakSquadComponent* Squad = PlayerState ? PlayerState->GetSquadComponent() : nullptr)
	{
		Checkpoint.Squad = OutbreakSquadSnapshot::CaptureSquad(*Squad, HostSelection);
	}
	Campaign->RecordCheckpoint(Checkpoint);
}

void AOutbreakGameMode::HandleMissionEnded(const FOutbreakMissionResults& Results)
{
	UOutbreakCampaignSubsystem* Campaign = GetCampaign();
	if (!bCampaignMission || !Campaign)
	{
		return;
	}
	const FOutbreakCampaignRecordResult Recorded = Campaign->RecordMissionResults(Results);
	if (AOutbreakGameState* OutbreakGameState = GetGameState<AOutbreakGameState>())
	{
		// Before the final status is published, so the summary shows the campaign options with it.
		OutbreakGameState->SetCampaignStatus(Campaign->BuildStatus(&Recorded));
	}
}

void AOutbreakGameMode::PauseForBriefing(APlayerController* Player)
{
	if (bPausedForBriefing || !Player || !ActiveMission || !ActiveMission->bPauseDuringBriefing)
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
