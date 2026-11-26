#include "Game/OutbreakGameMode.h"

#include "Combat/OutbreakTeam.h"
#include "Engine/World.h"
#include "Game/OutbreakGameState.h"
#include "Game/OutbreakPlayerState.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Player/OutbreakPlayerController.h"
#include "Squad/OutbreakSquadComponent.h"

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

void AOutbreakGameMode::StartPlay()
{
	Super::StartPlay();

	if (UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>())
	{
		Mission->StartMission();
	}
}

void AOutbreakGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (!NewPlayer || !NewPlayer->GetPawn())
	{
		return;
	}

	if (UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>())
	{
		Mission->RegisterPlayer(NewPlayer->GetPawn());
	}

	AOutbreakPlayerState* PlayerState = NewPlayer->GetPlayerState<AOutbreakPlayerState>();
	UOutbreakSquadComponent* Squad = PlayerState ? PlayerState->GetSquadComponent() : nullptr;
	if (Squad && !Squad->IsSquadInitialized())
	{
		const UOutbreakSquadConfiguration* Configuration = MissionDefinition && MissionDefinition->SquadConfiguration
			? MissionDefinition->SquadConfiguration.Get()
			: SquadConfiguration.Get();
		const int32 SizeOverride = UGameplayStatics::GetIntOption(OptionsString, TEXT("SquadSize"), 0);
		Squad->InitializeSquad(Configuration, NewPlayer->GetPawn(), SizeOverride);
	}
}
