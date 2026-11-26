#include "Missions/OutbreakMissionTypes.h"

bool FOutbreakObjectiveStatus::Identical(const FOutbreakObjectiveStatus& Other) const
{
	return State == Other.State
		&& bOptional == Other.bOptional
		&& bHidden == Other.bHidden
		&& Description.EqualTo(Other.Description)
		&& Progress.EqualTo(Other.Progress);
}

bool FOutbreakMissionStatus::Identical(const FOutbreakMissionStatus& Other) const
{
	if (State != Other.State
		|| ObjectiveIndex != Other.ObjectiveIndex
		|| ObjectiveCount != Other.ObjectiveCount
		|| !ObjectiveDescription.EqualTo(Other.ObjectiveDescription)
		|| !ObjectiveProgress.EqualTo(Other.ObjectiveProgress)
		|| !ResultText.EqualTo(Other.ResultText)
		|| !MissionName.EqualTo(Other.MissionName)
		|| Objectives.Num() != Other.Objectives.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Objectives.Num(); ++Index)
	{
		if (!Objectives[Index].Identical(Other.Objectives[Index]))
		{
			return false;
		}
	}
	return true;
}
