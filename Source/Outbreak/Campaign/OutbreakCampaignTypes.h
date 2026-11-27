#pragma once

#include "CoreMinimal.h"
#include "Missions/OutbreakMissionCheckpoint.h"
#include "Missions/OutbreakMissionTypes.h"

#include "OutbreakCampaignTypes.generated.h"

class UOutbreakCampaignDefinition;

/** A campaign mission as the campaign screen shows it. */
UENUM(BlueprintType)
enum class EOutbreakCampaignMissionStatus : uint8
{
	Locked,
	Available,
	Completed
};

/** What the campaign remembers about one mission (Phase 14). */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakMissionRecord
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	FName MissionId;

	/** Completed at least once. Unlocking and campaign completion follow this. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	bool bCompleted = false;

	/** Runs that ended (completed or failed). */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	int32 Attempts = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	int32 Completions = 0;

	/** Shortest completed run, seconds. 0 = never completed. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	float BestDuration = 0.0f;

	/** Most optional objectives completed in one completed run. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	int32 BestOptionalObjectives = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	int32 OptionalObjectiveCount = 0;

	/** Completed or Failed; NotStarted before the first run ends. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	EOutbreakMissionState LastOutcome = EOutbreakMissionState::NotStarted;
};

/**
 * The campaign state (Phase 14): which missions are done, their records, and the checkpoint of a
 * mission in progress. This is exactly what a save holds; unlocking and completion are always
 * derived from it, never stored separately.
 */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakCampaignProgress
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	FName CampaignId;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	TArray<FOutbreakMissionRecord> Missions;

	/** Mission in progress, saved at its last checkpoint. Cleared when that mission is completed. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	FOutbreakMissionCheckpoint Checkpoint;

	/** Seconds of mission time over every ended run. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	float TotalMissionTime = 0.0f;

	/** When every required mission was first completed (UTC); default = not yet. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	FDateTime CompletedAt;

	const FOutbreakMissionRecord* FindRecord(FName MissionId) const;
	FOutbreakMissionRecord& FindOrAddRecord(FName MissionId);
	bool IsMissionCompleted(FName MissionId) const;
};

/**
 * The campaign as the mission level's summary screen needs it (Phase 14 mission progression).
 * Built on the server by the campaign subsystem and replicated through AOutbreakGameState.
 */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakCampaignStatus
{
	GENERATED_BODY()

	/** The level was launched as a campaign mission. False = a stand-alone mission (Phase 11 flow). */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bCampaignMission = false;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	FText CampaignName;

	/** Another mission is available: the summary offers Continue. */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bHasNextMission = false;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	FText NextMissionName;

	/** This mission has a checkpoint: the summary offers Last Checkpoint. */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bHasCheckpoint = false;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bCampaignComplete = false;

	/** This mission's result completed the campaign. */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bCampaignJustCompleted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	FText CompletionText;

	/** Missions this result made available. */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	TArray<FText> UnlockedMissionNames;
};

/** Outcome of recording a mission's results in the campaign. */
struct OUTBREAK_API FOutbreakCampaignRecordResult
{
	/** Missions that became available because of this result. */
	TArray<FName> NewlyUnlocked;

	/** This result completed the campaign. */
	bool bCampaignCompleted = false;

	/** The results were for a mission of the campaign and were recorded. */
	bool bRecorded = false;
};

/**
 * Campaign progression rules (Phase 14). Pure functions of the definition and the progress, so the
 * campaign screen, the summary screen and the tests agree.
 */
namespace OutbreakCampaignRules
{
	OUTBREAK_API bool IsMissionUnlocked(const UOutbreakCampaignDefinition& Campaign, const FOutbreakCampaignProgress& Progress, int32 MissionIndex);

	OUTBREAK_API EOutbreakCampaignMissionStatus GetMissionStatus(const UOutbreakCampaignDefinition& Campaign,
		const FOutbreakCampaignProgress& Progress, int32 MissionIndex);

	/** Every required mission is completed. */
	OUTBREAK_API bool IsCampaignComplete(const UOutbreakCampaignDefinition& Campaign, const FOutbreakCampaignProgress& Progress);

	/** The first required mission that is available and not completed, else the first such side mission, else None. */
	OUTBREAK_API FName FindNextMission(const UOutbreakCampaignDefinition& Campaign, const FOutbreakCampaignProgress& Progress);

	/** Required missions completed / required missions. */
	OUTBREAK_API void CountRequired(const UOutbreakCampaignDefinition& Campaign, const FOutbreakCampaignProgress& Progress,
		int32& OutCompleted, int32& OutTotal);

	/**
	 * Records final Results for campaign mission MissionId: attempts, completion, best time and
	 * optional objectives, total time. Completing the checkpointed mission clears its checkpoint.
	 * Sets CompletedAt (to Now) the first time the campaign is complete.
	 */
	OUTBREAK_API FOutbreakCampaignRecordResult RecordResults(const UOutbreakCampaignDefinition& Campaign, FOutbreakCampaignProgress& Progress,
		FName MissionId, const FOutbreakMissionResults& Results, const FDateTime& Now);
}
