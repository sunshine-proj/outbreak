#include "Game/OutbreakGameMode.h"

#include "Player/OutbreakPlayerCharacter.h"
#include "Player/OutbreakPlayerController.h"

AOutbreakGameMode::AOutbreakGameMode()
{
	DefaultPawnClass = AOutbreakPlayerCharacter::StaticClass();
	PlayerControllerClass = AOutbreakPlayerController::StaticClass();
}
