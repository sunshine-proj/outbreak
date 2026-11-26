#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "OutbreakGameMode.generated.h"

/**
 * Default game mode: spawns the player as AOutbreakPlayerCharacter at a Player Start,
 * controlled by AOutbreakPlayerController.
 *
 * Create a Blueprint subclass to point DefaultPawnClass at the player character Blueprint
 * that has its input configuration assigned (see Docs/Phase1-EditorSetup.md).
 */
UCLASS()
class OUTBREAK_API AOutbreakGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AOutbreakGameMode();
};
