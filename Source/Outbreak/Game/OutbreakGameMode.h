#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "OutbreakGameMode.generated.h"

class UOutbreakSquadConfiguration;

/**
 * Default game mode: spawns the player as AOutbreakPlayerCharacter at a Player Start, controlled
 * by AOutbreakPlayerController, and builds the player's squad the first time they spawn.
 *
 * Create a Blueprint subclass to point DefaultPawnClass at the player character Blueprint
 * that has its input configuration assigned (see Docs/Phase1-EditorSetup.md), and to set the
 * squad configuration.
 *
 * For quick test runs, the URL option ?SquadSize=N overrides the configuration's size
 * (still bounded by the limits), e.g. `open L_Phase3_Squad?SquadSize=6`.
 */
UCLASS()
class OUTBREAK_API AOutbreakGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AOutbreakGameMode();

	virtual void RestartPlayer(AController* NewPlayer) override;

protected:

	/** The mission's squad. Empty = project default (Project Settings → Outbreak Squad). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Squad")
	TObjectPtr<UOutbreakSquadConfiguration> SquadConfiguration;
};
