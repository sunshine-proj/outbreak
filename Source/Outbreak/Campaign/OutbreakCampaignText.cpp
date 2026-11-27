#include "Campaign/OutbreakCampaignText.h"

#include "Campaign/OutbreakCampaignSaveGame.h"
#include "Missions/OutbreakObjectiveQueries.h"

#define LOCTEXT_NAMESPACE "OutbreakCampaignText"

namespace OutbreakCampaignText
{
	FSummaryOptions GetSummaryOptions(const FOutbreakCampaignStatus& Status, const FOutbreakMissionResults& Results)
	{
		FSummaryOptions Options;
		if (Status.bCampaignMission)
		{
			Options.bContinue = Results.Outcome == EOutbreakMissionState::Completed && Status.bHasNextMission;
			Options.bCheckpoint = Results.Outcome == EOutbreakMissionState::Failed && Status.bHasCheckpoint;
		}
		return Options;
	}

	FText BuildSummaryCampaignText(const FOutbreakCampaignStatus& Status, const FOutbreakMissionResults& Results)
	{
		if (!Status.bCampaignMission)
		{
			return FText::GetEmpty();
		}

		TArray<FText> Lines;
		if (Status.bCampaignJustCompleted)
		{
			Lines.Add(FText::Format(LOCTEXT("CampaignComplete", "CAMPAIGN COMPLETE: {0}"), Status.CampaignName));
			if (!Status.CompletionText.IsEmpty())
			{
				Lines.Add(Status.CompletionText);
			}
		}
		if (!Status.UnlockedMissionNames.IsEmpty())
		{
			Lines.Add(FText::Format(LOCTEXT("Unlocked", "Unlocked: {0}"), FText::Join(FText::FromString(TEXT(", ")), Status.UnlockedMissionNames)));
		}
		if (Results.Outcome == EOutbreakMissionState::Completed && Status.bHasNextMission)
		{
			Lines.Add(FText::Format(LOCTEXT("Next", "Next mission: {0}"), Status.NextMissionName));
		}
		if (Results.Outcome == EOutbreakMissionState::Failed && Status.bHasCheckpoint)
		{
			Lines.Add(LOCTEXT("CheckpointAvailable", "You can retry from the last checkpoint."));
		}
		return FText::Join(FText::FromString(TEXT("\n")), Lines);
	}

	FText BuildMissionLine(int32 MissionIndex, const FText& MissionName, EOutbreakCampaignMissionStatus Status,
		const FOutbreakMissionRecord* Record, bool bHasCheckpoint, const TArray<FText>& RequiredNames, bool bOptional)
	{
		const FText Name = bOptional
			? FText::Format(LOCTEXT("SideName", "{0}. {1} (side mission)"), MissionIndex + 1, MissionName)
			: FText::Format(LOCTEXT("Name", "{0}. {1}"), MissionIndex + 1, MissionName);

		FText State;
		switch (Status)
		{
		case EOutbreakCampaignMissionStatus::Completed:
		{
			TArray<FText> Parts = { LOCTEXT("Completed", "Completed") };
			if (Record && Record->BestDuration > 0.0f)
			{
				Parts.Add(FText::Format(LOCTEXT("Best", "best {0}"), OutbreakObjectiveQueries::FormatTime(Record->BestDuration)));
			}
			if (Record && Record->OptionalObjectiveCount > 0)
			{
				Parts.Add(FText::Format(LOCTEXT("Optional", "optional {0} / {1}"), Record->BestOptionalObjectives, Record->OptionalObjectiveCount));
			}
			State = FText::Join(FText::FromString(TEXT(", ")), Parts);
			break;
		}
		case EOutbreakCampaignMissionStatus::Available:
			State = Record && Record->Attempts > 0
				? FText::Format(LOCTEXT("AvailableTried", "Available, {0} attempt(s)"), Record->Attempts)
				: LOCTEXT("Available", "Available");
			break;
		default:
			State = RequiredNames.IsEmpty()
				? LOCTEXT("Locked", "Locked")
				: FText::Format(LOCTEXT("LockedBy", "Locked - complete {0} first"), FText::Join(FText::FromString(TEXT(", ")), RequiredNames));
			break;
		}
		if (bHasCheckpoint)
		{
			State = FText::Format(LOCTEXT("InProgress", "{0}, in progress (checkpoint)"), State);
		}
		return FText::Format(LOCTEXT("Line", "{0} - {1}"), Name, State);
	}

	FText BuildSlotLine(const FOutbreakSaveSlotSummary& Summary)
	{
		const FText Slot = Summary.SlotIndex == 0
			? LOCTEXT("Autosave", "Autosave")
			: FText::Format(LOCTEXT("Slot", "Slot {0}"), Summary.SlotIndex);
		if (!Summary.bExists)
		{
			return FText::Format(LOCTEXT("SlotEmpty", "{0} - empty"), Slot);
		}
		if (Summary.bUnreadable)
		{
			return FText::Format(LOCTEXT("SlotUnreadable", "{0} - unreadable (damaged, newer version or another campaign)"), Slot);
		}

		TArray<FText> Parts;
		Parts.Add(Summary.bCampaignComplete ? LOCTEXT("SlotComplete", "Campaign complete")
			: (Summary.CurrentMissionName.IsEmpty() ? LOCTEXT("SlotNoMission", "No mission available") : Summary.CurrentMissionName));
		Parts.Add(FText::Format(LOCTEXT("SlotMissions", "{0} / {1} missions"), Summary.CompletedMissions, Summary.RequiredMissions));
		if (Summary.bHasCheckpoint)
		{
			Parts.Add(LOCTEXT("SlotCheckpoint", "checkpoint"));
		}
		if (Summary.SavedAt != FDateTime())
		{
			Parts.Add(FText::AsDateTime(Summary.SavedAt, EDateTimeStyle::Short, EDateTimeStyle::Short));
		}
		return FText::Format(LOCTEXT("SlotLine", "{0} - {1}"), Slot, FText::Join(FText::FromString(TEXT(", ")), Parts));
	}

	FText BuildProgressLine(int32 CompletedRequired, int32 TotalRequired, float TotalMissionTime)
	{
		const FText Time = OutbreakObjectiveQueries::FormatTime(TotalMissionTime);
		return TotalRequired > 0 && CompletedRequired >= TotalRequired
			? FText::Format(LOCTEXT("ProgressComplete", "Campaign complete, mission time {0}"), Time)
			: FText::Format(LOCTEXT("Progress", "{0} / {1} missions completed, mission time {2}"), CompletedRequired, TotalRequired, Time);
	}
}

#undef LOCTEXT_NAMESPACE
