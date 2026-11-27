#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "OutbreakMissionDefinition.generated.h"

class UOutbreakObjective;
class UOutbreakSquadConfiguration;

/**
 * A mission: identity, briefing, deployment, squad, objectives and failure rules. Set on the game
 * mode (AOutbreakGameMode::MissionDefinition).
 *
 * Flow (Phase 11): briefing with squad and equipment selection → deployment at the starting
 * location → objectives (with dependencies, optional ones alongside) → usually an Extraction
 * objective last → mission summary.
 */
UCLASS(BlueprintType, Const)
class OUTBREAK_API UOutbreakMissionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	bool Validate(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/** Stable identifier for campaign progress and results (Phase 14). Empty = the asset name. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission")
	FName MissionId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission")
	FText DisplayName;

	/** One or two sentences, shown under the title in the briefing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission", meta = (MultiLine = "true"))
	FText Briefing;

	/** The mission's squad. Empty = the game mode's or project default configuration. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Squad")
	TObjectPtr<UOutbreakSquadConfiguration> SquadConfiguration;

	/**
	 * Show the briefing and squad/equipment selection before deployment. Off = deploy at once with
	 * the configuration's default squad (the Phase 6 behaviour). The URL option ?SkipBriefing does the same.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flow")
	bool bShowBriefing = true;

	/** Pause the game while the briefing is open, so enemies do not act before deployment. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flow", meta = (EditCondition = "bShowBriefing"))
	bool bPauseDuringBriefing = true;

	/**
	 * Starting location: AOutbreakMissionMarker::MarkerId where players deploy (facing the marker's
	 * rotation) and the squad forms up. Empty = where the player spawned (the Player Start).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flow")
	FName DeploymentMarkerId;

	/**
	 * Environment configuration: hour of day at mission start (0..24), applied to the level's
	 * AOutbreakTimeOfDay. Negative = keep the level's own start hour.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Environment", meta = (ClampMin = "-1.0", ClampMax = "24.0"))
	float StartHour = -1.0f;

	/**
	 * Alarm (Phase 13): an enemy that detects the squad raises the alarm after Alarm Delay unless it
	 * is silenced first. Encounters can bring reinforcements on the alarm. Off = no alarm.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Alarm")
	bool bAlarmEnabled = true;

	/** Seconds between an enemy detecting the squad and the alarm going up: the window to silence it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Alarm", meta = (ClampMin = "0.0", Units = "s", EditCondition = "bAlarmEnabled"))
	float AlarmDelay = 4.0f;

	/** Sideways spacing between players deploying at the same marker (co-op). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flow", meta = (ClampMin = "0.0", Units = "cm"))
	float DeploymentSpacing = 150.0f;

	/**
	 * The mission's objectives. Each activates by its own Activation rule (by default after the
	 * previous required one). The mission completes when every required objective has completed.
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Objectives")
	TArray<TObjectPtr<UOutbreakObjective>> Objectives;

	/** Fail when every player is dead (in single-player: when the player dies). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Failure")
	bool bFailOnPlayerDeath = true;

	/**
	 * Seconds after the end before the level restarts by itself. 0 = no automatic restart: the
	 * summary screen offers Retry and Quit.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flow", meta = (ClampMin = "0.0", Units = "s"))
	float RestartDelay = 0.0f;

	/**
	 * Campaign checkpoints (Phase 14): completing a required objective saves the objective states,
	 * mission time, alarm and the squad's health, weapons and items to the campaign autosave. Loading
	 * resumes there with the level reset. Off = a failed or quit mission restarts from the briefing.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flow")
	bool bCheckpointsEnabled = true;

	/** Seconds between objective evaluations. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flow", meta = (ClampMin = "0.05", Units = "s"))
	float UpdateInterval = 0.25f;

	/** MissionId, or the asset name when it is empty. */
	FName GetResolvedMissionId() const;
};
