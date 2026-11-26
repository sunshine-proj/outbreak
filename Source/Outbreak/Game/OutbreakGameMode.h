#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "OutbreakGameMode.generated.h"

class APlayerController;
class UOutbreakMissionDefinition;
class UOutbreakSquadConfiguration;
struct FOutbreakDeploymentSelection;

/**
 * Default game mode: spawns the player as AOutbreakPlayerCharacter at a Player Start, controlled
 * by AOutbreakPlayerController, and runs the mission (UOutbreakMissionSubsystem) when
 * MissionDefinition is set.
 *
 * Mission flow (Phase 11): when the mission shows a briefing, players wait in it (the game paused
 * if the mission says so) until they deploy with their squad and equipment selection; the squad is
 * built at deployment, at the mission's starting location. The mission starts once every player has
 * deployed. Without a briefing (or with ?SkipBriefing, or in free play) players deploy at once with
 * the default squad. After the mission, Retry on the summary screen replays the level.
 *
 * Create a Blueprint subclass to point DefaultPawnClass at the player character Blueprint
 * that has its input configuration assigned (see Docs/Phase1-EditorSetup.md), and to set the
 * squad configuration.
 *
 * For quick test runs, the URL option ?SquadSize=N overrides the configuration's size when there is
 * no briefing (still bounded by the limits), e.g. `open L_Phase3_Squad?SquadSize=6?SkipBriefing`.
 */
UCLASS()
class OUTBREAK_API AOutbreakGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AOutbreakGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void StartPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** A player left the briefing with Selection (from AOutbreakPlayerController). Ignored once deployed. */
	void HandleDeployRequest(APlayerController* Player, const FOutbreakDeploymentSelection& Selection);

	/** A player asked to replay the mission from the summary screen. */
	void HandleRestartRequest(APlayerController* Player);

	/** The mission shows a briefing before deployment. */
	bool ShouldRunBriefing() const;

protected:

	/** The squad when no mission sets one. Empty = project default (Project Settings → Outbreak Squad). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Squad")
	TObjectPtr<UOutbreakSquadConfiguration> SquadConfiguration;

	/** The mission to run. Empty = free play (no objectives, no failure). Its squad configuration wins. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission")
	TObjectPtr<UOutbreakMissionDefinition> MissionDefinition;

private:

	/** Moves the player to the starting location, builds its squad (Selection null = default squad) and reports it deployed. */
	void DeployPlayer(AController* Player, const FOutbreakDeploymentSelection* Selection);

	/** The mission's squad configuration, else the game mode's, else the project default (may be null). */
	UOutbreakSquadConfiguration* ResolveSquadConfiguration() const;

	void PauseForBriefing(APlayerController* Player);
	void ResumeAfterBriefing();

	bool bPausedForBriefing = false;
};
