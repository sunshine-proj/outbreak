#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "OutbreakGameMode.generated.h"

class UOutbreakMissionDefinition;
class UOutbreakSquadConfiguration;

/**
 * Default game mode: spawns the player as AOutbreakPlayerCharacter at a Player Start, controlled
 * by AOutbreakPlayerController, builds the player's squad the first time they spawn, and runs the
 * mission (UOutbreakMissionSubsystem) when MissionDefinition is set.
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

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;

protected:

	/** The squad when no mission sets one. Empty = project default (Project Settings → Outbreak Squad). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Squad")
	TObjectPtr<UOutbreakSquadConfiguration> SquadConfiguration;

	/** The mission to run. Empty = free play (no objectives, no failure). Its squad configuration wins. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission")
	TObjectPtr<UOutbreakMissionDefinition> MissionDefinition;
};
