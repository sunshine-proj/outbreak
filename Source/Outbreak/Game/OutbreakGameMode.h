#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Missions/OutbreakMissionCheckpoint.h"
#include "Squad/OutbreakSquadSelection.h"

#include "OutbreakGameMode.generated.h"

class APlayerController;
class UOutbreakCampaignSubsystem;
class UOutbreakMissionDefinition;
class UOutbreakSquadConfiguration;
struct FOutbreakMissionResults;

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
 *
 * Campaign (Phase 14): when UOutbreakCampaignSubsystem launched a mission on this level, that
 * mission runs instead of MissionDefinition; checkpoints and results go back to the campaign, and a
 * launch from a checkpoint skips the briefing, rebuilds the saved squad at the checkpoint marker and
 * resumes the objectives. Opened directly (not from the campaign), the level runs as before.
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

	/** A player asked to replay the mission from the summary screen; in a campaign, optionally from its last checkpoint. */
	void HandleRestartRequest(APlayerController* Player, bool bFromCheckpoint = false);

	/** The summary's Continue: the next campaign mission. Ignored outside a campaign or before the end. */
	void HandleContinueRequest(APlayerController* Player);

	/** The summary's Main Menu: back to the campaign screen (quits outside a campaign). */
	void HandleMainMenuRequest(APlayerController* Player);

	/** The mission this level runs: the campaign's launched mission, else MissionDefinition. Valid from InitGame. */
	UOutbreakMissionDefinition* GetActiveMissionDefinition() const { return ActiveMission; }

	/** This level was launched as a campaign mission. */
	bool IsCampaignMission() const { return bCampaignMission; }

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

	UOutbreakCampaignSubsystem* GetCampaign() const;

	/** Resume: restores the saved squad state and the alarm, then starts the objectives from the checkpoint. */
	void StartFromCheckpoint();

	UFUNCTION()
	void HandleCheckpointReached(int32 CompletedObjectiveIndex);

	UFUNCTION()
	void HandleMissionEnded(const FOutbreakMissionResults& Results);

	/** The mission's squad configuration, else the game mode's, else the project default (may be null). */
	UOutbreakSquadConfiguration* ResolveSquadConfiguration() const;

	void PauseForBriefing(APlayerController* Player);
	void ResumeAfterBriefing();

	bool bPausedForBriefing = false;

	/** Resolved in InitGame. */
	UPROPERTY(Transient)
	TObjectPtr<UOutbreakMissionDefinition> ActiveMission;

	bool bCampaignMission = false;

	/** The checkpoint this launch resumes (a copy: the campaign's may be replaced during play). */
	FOutbreakMissionCheckpoint ResumeCheckpoint;

	/** The first deployed player's squad selection, for checkpoints. Keeps the chosen loadouts referenced. */
	UPROPERTY(Transient)
	FOutbreakDeploymentSelection HostSelection;

	TWeakObjectPtr<AController> HostPlayer;
};
