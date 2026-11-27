#include "Campaign/OutbreakCampaignDefinition.h"

#include "Missions/OutbreakMissionDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "OutbreakCampaignDefinition"

bool UOutbreakCampaignDefinition::Validate(TArray<FText>& OutErrors) const
{
	const int32 InitialErrorCount = OutErrors.Num();

	if (Missions.IsEmpty())
	{
		OutErrors.Add(LOCTEXT("NoMissions", "A campaign needs at least one mission."));
		return false;
	}

	TSet<FName> SeenIds;
	TSet<FName> RequiredIds;
	bool bAnyRequired = false;
	bool bAnyStarting = false;
	bool bRequiredBefore = false;

	for (int32 Index = 0; Index < Missions.Num(); ++Index)
	{
		const FOutbreakCampaignMission& Entry = Missions[Index];
		const FText Label = Entry.MissionId.IsNone() ? FText::AsNumber(Index + 1) : FText::FromName(Entry.MissionId);

		if (Entry.MissionId.IsNone())
		{
			OutErrors.Add(FText::Format(LOCTEXT("NoId", "Campaign mission {0} has no Mission Id."), Label));
		}
		else if (SeenIds.Contains(Entry.MissionId))
		{
			OutErrors.Add(FText::Format(LOCTEXT("DuplicateId", "Campaign mission ID '{0}' is used twice."), Label));
		}
		if (Entry.Mission.IsNull())
		{
			OutErrors.Add(FText::Format(LOCTEXT("NoMission", "Campaign mission {0} has no mission definition."), Label));
		}
		if (Entry.Map.IsNull())
		{
			OutErrors.Add(FText::Format(LOCTEXT("NoMap", "Campaign mission {0} has no map."), Label));
		}

		switch (Entry.Unlock)
		{
		case EOutbreakMissionUnlock::AtCampaignStart:
			bAnyStarting = true;
			break;
		case EOutbreakMissionUnlock::AfterPrevious:
			// With no earlier required mission, AfterPrevious is available from the start.
			bAnyStarting |= !bRequiredBefore;
			break;
		case EOutbreakMissionUnlock::AfterMissions:
			if (Entry.RequiredMissions.IsEmpty())
			{
				OutErrors.Add(FText::Format(LOCTEXT("NoRequired", "Campaign mission {0} unlocks after missions but lists none."), Label));
			}
			for (const FName Required : Entry.RequiredMissions)
			{
				// Only earlier missions, so unlocking can never form a cycle.
				if (!SeenIds.Contains(Required))
				{
					OutErrors.Add(FText::Format(LOCTEXT("UnknownRequired", "Campaign mission {0} requires '{1}', which is not an earlier mission."),
						Label, FText::FromName(Required)));
				}
				else if (!Entry.bOptional && !RequiredIds.Contains(Required))
				{
					OutErrors.Add(FText::Format(LOCTEXT("RequiredOnOptional", "Required campaign mission {0} cannot depend on side mission '{1}'."),
						Label, FText::FromName(Required)));
				}
			}
			break;
		}

		if (!Entry.MissionId.IsNone())
		{
			SeenIds.Add(Entry.MissionId);
			if (!Entry.bOptional)
			{
				RequiredIds.Add(Entry.MissionId);
			}
		}
		if (!Entry.bOptional)
		{
			bAnyRequired = true;
			bRequiredBefore = true;
		}
	}

	if (!bAnyRequired)
	{
		OutErrors.Add(LOCTEXT("NoRequiredMission", "A campaign needs at least one required (non-side) mission."));
	}
	if (!bAnyStarting)
	{
		OutErrors.Add(LOCTEXT("NoStartingMission", "No campaign mission is available at the start."));
	}

	return OutErrors.Num() == InitialErrorCount;
}

FName UOutbreakCampaignDefinition::GetResolvedCampaignId() const
{
	return CampaignId.IsNone() ? GetFName() : CampaignId;
}

int32 UOutbreakCampaignDefinition::FindMissionIndex(FName MissionId) const
{
	if (MissionId.IsNone())
	{
		return INDEX_NONE;
	}
	return Missions.IndexOfByPredicate([MissionId](const FOutbreakCampaignMission& Entry) { return Entry.MissionId == MissionId; });
}

const FOutbreakCampaignMission* UOutbreakCampaignDefinition::FindMission(FName MissionId) const
{
	const int32 Index = FindMissionIndex(MissionId);
	return Missions.IsValidIndex(Index) ? &Missions[Index] : nullptr;
}

#if WITH_EDITOR
EDataValidationResult UOutbreakCampaignDefinition::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Result = Super::IsDataValid(Context);

	TArray<FText> Errors;
	Validate(Errors);
	for (const FOutbreakCampaignMission& Entry : Missions)
	{
		const UOutbreakMissionDefinition* Mission = Entry.Mission.LoadSynchronous();
		if (!Mission)
		{
			continue;
		}
		Mission->Validate(Errors);
		if (Mission->GetResolvedMissionId() != Entry.MissionId)
		{
			Errors.Add(FText::Format(LOCTEXT("IdMismatch", "Campaign mission '{0}' uses mission {1}, whose Mission Id is '{2}'."),
				FText::FromName(Entry.MissionId), FText::FromString(Mission->GetName()), FText::FromName(Mission->GetResolvedMissionId())));
		}
	}

	if (!Errors.IsEmpty())
	{
		for (const FText& Error : Errors)
		{
			Context.AddError(Error);
		}
		return EDataValidationResult::Invalid;
	}
	return CombineDataValidationResults(Result, EDataValidationResult::Valid);
}
#endif

#undef LOCTEXT_NAMESPACE
