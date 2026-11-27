#pragma once

#include "CoreMinimal.h"
#include "Campaign/OutbreakCampaignTypes.h"

struct FOutbreakSaveSlotSummary;

/**
 * Text for the campaign screen and the campaign part of the mission summary (Phase 14). Pure
 * functions, so the default C++ widgets and Widget Blueprints say the same thing, testably.
 */
namespace OutbreakCampaignText
{
	/** Which campaign buttons the summary shows. */
	struct FSummaryOptions
	{
		/** Completed campaign mission with another one available. */
		bool bContinue = false;
		/** Failed campaign mission that has a checkpoint. */
		bool bCheckpoint = false;
	};

	OUTBREAK_API FSummaryOptions GetSummaryOptions(const FOutbreakCampaignStatus& Status, const FOutbreakMissionResults& Results);

	/** Unlocked missions, the next mission, campaign completion; empty for a stand-alone mission. */
	OUTBREAK_API FText BuildSummaryCampaignText(const FOutbreakCampaignStatus& Status, const FOutbreakMissionResults& Results);

	/**
	 * One row of the campaign mission list, e.g. "2. Relay Station - Completed, best 12:34,
	 * optional 1 / 2", "Available", "In progress (checkpoint)" or "Locked - complete Night Raid first".
	 * RequiredNames: display names of the missions that unlock it (for Locked rows).
	 */
	OUTBREAK_API FText BuildMissionLine(int32 MissionIndex, const FText& MissionName, EOutbreakCampaignMissionStatus Status,
		const FOutbreakMissionRecord* Record, bool bHasCheckpoint, const TArray<FText>& RequiredNames, bool bOptional);

	/** "Autosave - Relay Station, 1 / 3 missions, checkpoint, 2026-11-27 14:02", "Slot 2 - empty". */
	OUTBREAK_API FText BuildSlotLine(const FOutbreakSaveSlotSummary& Summary);

	/** "3 / 5 missions completed", or "Campaign complete" with the total mission time. */
	OUTBREAK_API FText BuildProgressLine(int32 CompletedRequired, int32 TotalRequired, float TotalMissionTime);
}
