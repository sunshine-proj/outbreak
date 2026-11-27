#include "Campaign/OutbreakCampaignTypes.h"

#include "Campaign/OutbreakCampaignDefinition.h"

const FOutbreakMissionRecord* FOutbreakCampaignProgress::FindRecord(FName MissionId) const
{
	return Missions.FindByPredicate([MissionId](const FOutbreakMissionRecord& Record) { return Record.MissionId == MissionId; });
}

FOutbreakMissionRecord& FOutbreakCampaignProgress::FindOrAddRecord(FName MissionId)
{
	if (FOutbreakMissionRecord* Existing = Missions.FindByPredicate([MissionId](const FOutbreakMissionRecord& Record) { return Record.MissionId == MissionId; }))
	{
		return *Existing;
	}
	FOutbreakMissionRecord& Record = Missions.AddDefaulted_GetRef();
	Record.MissionId = MissionId;
	return Record;
}

bool FOutbreakCampaignProgress::IsMissionCompleted(FName MissionId) const
{
	const FOutbreakMissionRecord* Record = FindRecord(MissionId);
	return Record && Record->bCompleted;
}

namespace OutbreakCampaignRules
{
	bool IsMissionUnlocked(const UOutbreakCampaignDefinition& Campaign, const FOutbreakCampaignProgress& Progress, int32 MissionIndex)
	{
		if (!Campaign.Missions.IsValidIndex(MissionIndex))
		{
			return false;
		}
		const FOutbreakCampaignMission& Entry = Campaign.Missions[MissionIndex];
		switch (Entry.Unlock)
		{
		case EOutbreakMissionUnlock::AtCampaignStart:
			return true;

		case EOutbreakMissionUnlock::AfterMissions:
			for (const FName Required : Entry.RequiredMissions)
			{
				if (!Progress.IsMissionCompleted(Required))
				{
					return false;
				}
			}
			return true;

		case EOutbreakMissionUnlock::AfterPrevious:
		default:
			for (int32 Index = MissionIndex - 1; Index >= 0; --Index)
			{
				if (!Campaign.Missions[Index].bOptional)
				{
					return Progress.IsMissionCompleted(Campaign.Missions[Index].MissionId);
				}
			}
			return true;
		}
	}

	EOutbreakCampaignMissionStatus GetMissionStatus(const UOutbreakCampaignDefinition& Campaign,
		const FOutbreakCampaignProgress& Progress, int32 MissionIndex)
	{
		if (!Campaign.Missions.IsValidIndex(MissionIndex))
		{
			return EOutbreakCampaignMissionStatus::Locked;
		}
		// A completed mission stays completed (replayable) even if its rules changed in a later version.
		if (Progress.IsMissionCompleted(Campaign.Missions[MissionIndex].MissionId))
		{
			return EOutbreakCampaignMissionStatus::Completed;
		}
		return IsMissionUnlocked(Campaign, Progress, MissionIndex) ? EOutbreakCampaignMissionStatus::Available : EOutbreakCampaignMissionStatus::Locked;
	}

	void CountRequired(const UOutbreakCampaignDefinition& Campaign, const FOutbreakCampaignProgress& Progress, int32& OutCompleted, int32& OutTotal)
	{
		OutCompleted = 0;
		OutTotal = 0;
		for (const FOutbreakCampaignMission& Entry : Campaign.Missions)
		{
			if (!Entry.bOptional)
			{
				++OutTotal;
				OutCompleted += Progress.IsMissionCompleted(Entry.MissionId) ? 1 : 0;
			}
		}
	}

	bool IsCampaignComplete(const UOutbreakCampaignDefinition& Campaign, const FOutbreakCampaignProgress& Progress)
	{
		int32 Completed = 0;
		int32 Total = 0;
		CountRequired(Campaign, Progress, Completed, Total);
		return Total > 0 && Completed == Total;
	}

	FName FindNextMission(const UOutbreakCampaignDefinition& Campaign, const FOutbreakCampaignProgress& Progress)
	{
		FName FirstSideMission;
		for (int32 Index = 0; Index < Campaign.Missions.Num(); ++Index)
		{
			if (GetMissionStatus(Campaign, Progress, Index) != EOutbreakCampaignMissionStatus::Available)
			{
				continue;
			}
			const FOutbreakCampaignMission& Entry = Campaign.Missions[Index];
			if (!Entry.bOptional)
			{
				return Entry.MissionId;
			}
			if (FirstSideMission.IsNone())
			{
				FirstSideMission = Entry.MissionId;
			}
		}
		return FirstSideMission;
	}

	FOutbreakCampaignRecordResult RecordResults(const UOutbreakCampaignDefinition& Campaign, FOutbreakCampaignProgress& Progress,
		FName MissionId, const FOutbreakMissionResults& Results, const FDateTime& Now)
	{
		FOutbreakCampaignRecordResult Outcome;
		if (Campaign.FindMissionIndex(MissionId) == INDEX_NONE || !Results.IsFinal())
		{
			return Outcome;
		}
		Outcome.bRecorded = true;

		const bool bWasComplete = IsCampaignComplete(Campaign, Progress);
		TArray<EOutbreakCampaignMissionStatus> StatusBefore;
		for (int32 Index = 0; Index < Campaign.Missions.Num(); ++Index)
		{
			StatusBefore.Add(GetMissionStatus(Campaign, Progress, Index));
		}

		FOutbreakMissionRecord& Record = Progress.FindOrAddRecord(MissionId);
		++Record.Attempts;
		Record.LastOutcome = Results.Outcome;
		Record.OptionalObjectiveCount = Results.OptionalObjectiveCount;
		Progress.TotalMissionTime += FMath::Max(0.0f, Results.Duration);

		if (Results.Outcome == EOutbreakMissionState::Completed)
		{
			++Record.Completions;
			Record.BestDuration = Record.bCompleted && Record.BestDuration > 0.0f
				? FMath::Min(Record.BestDuration, Results.Duration) : Results.Duration;
			Record.BestOptionalObjectives = FMath::Max(Record.BestOptionalObjectives, Results.OptionalObjectivesCompleted);
			Record.bCompleted = true;

			// The run is over; its checkpoint must not resume a finished mission.
			if (Progress.Checkpoint.MissionId == MissionId)
			{
				Progress.Checkpoint = FOutbreakMissionCheckpoint();
			}
		}

		for (int32 Index = 0; Index < Campaign.Missions.Num(); ++Index)
		{
			if (StatusBefore[Index] == EOutbreakCampaignMissionStatus::Locked
				&& GetMissionStatus(Campaign, Progress, Index) != EOutbreakCampaignMissionStatus::Locked)
			{
				Outcome.NewlyUnlocked.Add(Campaign.Missions[Index].MissionId);
			}
		}

		if (!bWasComplete && IsCampaignComplete(Campaign, Progress))
		{
			Outcome.bCampaignCompleted = true;
			if (Progress.CompletedAt == FDateTime())
			{
				Progress.CompletedAt = Now;
			}
		}
		return Outcome;
	}
}
