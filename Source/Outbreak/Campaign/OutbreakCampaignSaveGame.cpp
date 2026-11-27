#include "Campaign/OutbreakCampaignSaveGame.h"

#include "Campaign/OutbreakCampaignDefinition.h"
#include "Squad/OutbreakSquadSettings.h"

#define LOCTEXT_NAMESPACE "OutbreakSaveValidation"

namespace OutbreakSaveValidation
{
	namespace
	{
		/** Problems with a checkpoint that make it unusable. Mission-specific objective checks happen at resume. */
		void CheckCheckpoint(const FOutbreakCampaignProgress& Progress, const UOutbreakCampaignDefinition& Campaign, TArray<FText>& OutProblems)
		{
			const FOutbreakMissionCheckpoint& Checkpoint = Progress.Checkpoint;
			if (!Checkpoint.IsSet())
			{
				return;
			}
			const FText Mission = FText::FromName(Checkpoint.MissionId);
			const int32 MissionIndex = Campaign.FindMissionIndex(Checkpoint.MissionId);
			if (MissionIndex == INDEX_NONE)
			{
				OutProblems.Add(FText::Format(LOCTEXT("CheckpointUnknown", "The checkpoint is for mission '{0}', which is not in the campaign."), Mission));
				return;
			}
			if (!OutbreakCampaignRules::IsMissionUnlocked(Campaign, Progress, MissionIndex) && !Progress.IsMissionCompleted(Checkpoint.MissionId))
			{
				OutProblems.Add(FText::Format(LOCTEXT("CheckpointLocked", "The checkpoint is for mission '{0}', which is locked."), Mission));
			}
			if (Checkpoint.ObjectiveStates.IsEmpty())
			{
				OutProblems.Add(LOCTEXT("CheckpointNoObjectives", "The checkpoint has no objective states."));
			}
			if (!FMath::IsFinite(Checkpoint.MissionTime) || Checkpoint.MissionTime < 0.0f)
			{
				OutProblems.Add(LOCTEXT("CheckpointTime", "The checkpoint's mission time is invalid."));
			}

			const FOutbreakSquadSnapshot& Squad = Checkpoint.Squad;
			if (!Squad.IsEmpty())
			{
				const FOutbreakSquadSizeLimits& Limits = GetDefault<UOutbreakSquadSettings>()->SizeLimits;
				if (Squad.SquadSize < 1 || Squad.SquadSize > Limits.MaxSquadSize)
				{
					OutProblems.Add(FText::Format(LOCTEXT("CheckpointSquadSize", "The checkpoint squad has {0} soldiers; the limit is {1}."),
						Squad.SquadSize, Limits.MaxSquadSize));
				}
				if (Squad.Members.Num() > Squad.SquadSize || Squad.TeammateRosterIndices.Num() != FMath::Max(0, Squad.SquadSize - 1))
				{
					OutProblems.Add(LOCTEXT("CheckpointSquadShape", "The checkpoint squad's members do not match its size."));
				}
				for (const FOutbreakSoldierSnapshot& Member : Squad.Members)
				{
					if (!FMath::IsFinite(Member.HealthFraction) || Member.HealthFraction < 0.0f || Member.HealthFraction > 1.0f)
					{
						OutProblems.Add(LOCTEXT("CheckpointHealth", "A checkpoint soldier's health is out of range."));
						break;
					}
				}
			}
		}
	}

	bool ValidateForSave(const FOutbreakCampaignProgress& Progress, const UOutbreakCampaignDefinition& Campaign, TArray<FText>& OutErrors)
	{
		const int32 InitialErrorCount = OutErrors.Num();

		if (Progress.CampaignId != Campaign.GetResolvedCampaignId())
		{
			OutErrors.Add(FText::Format(LOCTEXT("SaveCampaignId", "The progress is for campaign '{0}', not '{1}'."),
				FText::FromName(Progress.CampaignId), FText::FromName(Campaign.GetResolvedCampaignId())));
		}

		TSet<FName> Seen;
		for (const FOutbreakMissionRecord& Record : Progress.Missions)
		{
			const FText Mission = FText::FromName(Record.MissionId);
			if (Campaign.FindMissionIndex(Record.MissionId) == INDEX_NONE)
			{
				OutErrors.Add(FText::Format(LOCTEXT("SaveUnknownMission", "Record for unknown mission '{0}'."), Mission));
			}
			if (Seen.Contains(Record.MissionId))
			{
				OutErrors.Add(FText::Format(LOCTEXT("SaveDuplicate", "Two records for mission '{0}'."), Mission));
			}
			Seen.Add(Record.MissionId);
			if (Record.Attempts < 0 || Record.Completions < 0 || Record.Completions > Record.Attempts
				|| Record.bCompleted != (Record.Completions > 0) || Record.BestDuration < 0.0f)
			{
				OutErrors.Add(FText::Format(LOCTEXT("SaveCounts", "Record for mission '{0}' has inconsistent counts."), Mission));
			}
		}
		if (Progress.TotalMissionTime < 0.0f || !FMath::IsFinite(Progress.TotalMissionTime))
		{
			OutErrors.Add(LOCTEXT("SaveTotalTime", "Total mission time is invalid."));
		}

		CheckCheckpoint(Progress, Campaign, OutErrors);
		return OutErrors.Num() == InitialErrorCount;
	}

	bool ValidateLoaded(USaveGame* Loaded, const UOutbreakCampaignDefinition& Campaign, TArray<FText>& OutErrors, TArray<FText>& OutWarnings)
	{
		UOutbreakCampaignSaveGame* Save = Cast<UOutbreakCampaignSaveGame>(Loaded);
		if (!Save)
		{
			OutErrors.Add(Loaded
				? FText::Format(LOCTEXT("LoadWrongClass", "The slot holds a {0}, not a campaign save."), FText::FromString(Loaded->GetClass()->GetName()))
				: LOCTEXT("LoadUnreadable", "The save could not be read (missing or corrupt)."));
			return false;
		}
		if (Save->SaveVersion > OutbreakSaveVersion::Current)
		{
			OutErrors.Add(FText::Format(LOCTEXT("LoadNewer", "The save is version {0}; this build reads up to version {1}."),
				Save->SaveVersion, OutbreakSaveVersion::Current));
			return false;
		}
		if (Save->SaveVersion < OutbreakSaveVersion::Initial)
		{
			OutErrors.Add(FText::Format(LOCTEXT("LoadInvalidVersion", "The save has an invalid version ({0})."), Save->SaveVersion));
			return false;
		}
		// Migrations from older versions go here, one step per version, before the checks below.

		FOutbreakCampaignProgress& Progress = Save->Progress;
		if (Progress.CampaignId != Campaign.GetResolvedCampaignId())
		{
			OutErrors.Add(FText::Format(LOCTEXT("LoadOtherCampaign", "The save is for campaign '{0}', not '{1}'."),
				FText::FromName(Progress.CampaignId), FText::FromName(Campaign.GetResolvedCampaignId())));
			return false;
		}

		// Records: keep known missions, merge duplicates, clamp counts.
		TArray<FOutbreakMissionRecord> Records;
		for (const FOutbreakMissionRecord& Record : Progress.Missions)
		{
			const FText Mission = FText::FromName(Record.MissionId);
			if (Campaign.FindMissionIndex(Record.MissionId) == INDEX_NONE)
			{
				OutWarnings.Add(FText::Format(LOCTEXT("LoadDropped", "Dropped the record of mission '{0}', which is no longer in the campaign."), Mission));
				continue;
			}

			FOutbreakMissionRecord Clean = Record;
			Clean.Attempts = FMath::Max(0, Clean.Attempts);
			Clean.Completions = FMath::Clamp(Clean.Completions, 0, Clean.Attempts);
			Clean.bCompleted = Clean.bCompleted || Clean.Completions > 0;
			Clean.Completions = Clean.bCompleted ? FMath::Max(1, Clean.Completions) : 0;
			Clean.Attempts = FMath::Max(Clean.Attempts, Clean.Completions);
			Clean.BestDuration = FMath::IsFinite(Clean.BestDuration) ? FMath::Max(0.0f, Clean.BestDuration) : 0.0f;
			Clean.BestOptionalObjectives = FMath::Max(0, Clean.BestOptionalObjectives);
			Clean.OptionalObjectiveCount = FMath::Max(0, Clean.OptionalObjectiveCount);
			if (Clean.Attempts != Record.Attempts || Clean.Completions != Record.Completions || Clean.bCompleted != Record.bCompleted
				|| Clean.BestDuration != Record.BestDuration)
			{
				OutWarnings.Add(FText::Format(LOCTEXT("LoadRepaired", "Repaired inconsistent counts in the record of mission '{0}'."), Mission));
			}

			if (FOutbreakMissionRecord* Existing = Records.FindByPredicate([&Clean](const FOutbreakMissionRecord& Other) { return Other.MissionId == Clean.MissionId; }))
			{
				OutWarnings.Add(FText::Format(LOCTEXT("LoadMerged", "Merged two records of mission '{0}'."), Mission));
				Existing->bCompleted |= Clean.bCompleted;
				Existing->Attempts += Clean.Attempts;
				Existing->Completions += Clean.Completions;
				Existing->BestOptionalObjectives = FMath::Max(Existing->BestOptionalObjectives, Clean.BestOptionalObjectives);
				if (Clean.BestDuration > 0.0f)
				{
					Existing->BestDuration = Existing->BestDuration > 0.0f ? FMath::Min(Existing->BestDuration, Clean.BestDuration) : Clean.BestDuration;
				}
				continue;
			}
			Records.Add(Clean);
		}
		Progress.Missions = MoveTemp(Records);

		if (!FMath::IsFinite(Progress.TotalMissionTime) || Progress.TotalMissionTime < 0.0f)
		{
			OutWarnings.Add(LOCTEXT("LoadTotalTime", "Reset an invalid total mission time."));
			Progress.TotalMissionTime = 0.0f;
		}

		// A checkpoint is a convenience: an unusable one is dropped, the progress is kept.
		TArray<FText> CheckpointProblems;
		CheckCheckpoint(Progress, Campaign, CheckpointProblems);
		if (!CheckpointProblems.IsEmpty())
		{
			OutWarnings.Append(CheckpointProblems);
			OutWarnings.Add(LOCTEXT("CheckpointDiscarded", "Discarded the checkpoint; the mission restarts from its briefing."));
			Progress.Checkpoint = FOutbreakMissionCheckpoint();
		}

		// Completion is derived, never trusted from the file.
		if (!OutbreakCampaignRules::IsCampaignComplete(Campaign, Progress))
		{
			Progress.CompletedAt = FDateTime();
		}
		Save->SaveVersion = OutbreakSaveVersion::Current;
		return true;
	}
}

#undef LOCTEXT_NAMESPACE
