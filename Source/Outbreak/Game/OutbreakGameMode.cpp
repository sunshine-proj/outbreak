#include "Game/OutbreakGameMode.h"

#include "Combat/OutbreakTeam.h"
#include "Game/OutbreakPlayerState.h"
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
}

void AOutbreakGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// AI perception uses the global solver to tell enemies from friends.
	FGenericTeamId::SetAttitudeSolver(&OutbreakTeam::GetAttitude);
}

void AOutbreakGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (!NewPlayer || !NewPlayer->GetPawn())
	{
		return;
	}

	AOutbreakPlayerState* PlayerState = NewPlayer->GetPlayerState<AOutbreakPlayerState>();
	UOutbreakSquadComponent* Squad = PlayerState ? PlayerState->GetSquadComponent() : nullptr;
	if (Squad && !Squad->IsSquadInitialized())
	{
		const int32 SizeOverride = UGameplayStatics::GetIntOption(OptionsString, TEXT("SquadSize"), 0);
		Squad->InitializeSquad(SquadConfiguration, NewPlayer->GetPawn(), SizeOverride);
	}
}
