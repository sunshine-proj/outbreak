#include "Game/OutbreakFrontEndGameMode.h"

#include "Player/OutbreakFrontEndPlayerController.h"

AOutbreakFrontEndGameMode::AOutbreakFrontEndGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AOutbreakFrontEndPlayerController::StaticClass();
}
