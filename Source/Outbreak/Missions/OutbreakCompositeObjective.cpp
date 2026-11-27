#include "Missions/OutbreakCompositeObjective.h"

#include "Missions/OutbreakMissionSubsystem.h"

#define LOCTEXT_NAMESPACE "OutbreakCompositeObjective"

int32 UOutbreakCompositeObjective::GetNeededCount() const
{
	const int32 Count = SubObjectives.Num();
	return RequiredCount <= 0 ? Count : FMath::Min(RequiredCount, Count);
}

int32 UOutbreakCompositeObjective::GetCompletedCount() const
{
	int32 Completed = 0;
	for (const UOutbreakObjective* Sub : SubObjectives)
	{
		Completed += Sub && Sub->GetState() == EOutbreakObjectiveState::Completed ? 1 : 0;
	}
	return Completed;
}

void UOutbreakCompositeObjective::Activate(UOutbreakMissionSubsystem& Mission)
{
	const float StartTime = Mission.GetMissionTime();
	for (UOutbreakObjective* Sub : SubObjectives)
	{
		if (Sub)
		{
			Sub->SetState(EOutbreakObjectiveState::Active);
			Sub->SetActivationTime(StartTime);
			Sub->Activate(Mission);
		}
	}
}

EOutbreakObjectiveState UOutbreakCompositeObjective::Evaluate(UOutbreakMissionSubsystem& Mission)
{
	int32 Completed = 0;
	int32 Failed = 0;
	for (UOutbreakObjective* Sub : SubObjectives)
	{
		if (!Sub)
		{
			++Failed;
			continue;
		}
		if (Sub->GetState() == EOutbreakObjectiveState::Active)
		{
			const EOutbreakObjectiveState Result = Sub->HasTimedOut(Mission.GetMissionTime())
				? EOutbreakObjectiveState::Failed : Sub->Evaluate(Mission);
			if (Result != EOutbreakObjectiveState::Active)
			{
				Sub->SetState(Result);
				Sub->Deactivate();
			}
		}
		Completed += Sub->GetState() == EOutbreakObjectiveState::Completed ? 1 : 0;
		Failed += Sub->GetState() == EOutbreakObjectiveState::Failed ? 1 : 0;
	}

	const int32 Needed = GetNeededCount();
	if (Completed >= Needed)
	{
		return EOutbreakObjectiveState::Completed;
	}
	if (SubObjectives.Num() - Failed < Needed)
	{
		return EOutbreakObjectiveState::Failed;
	}
	return EOutbreakObjectiveState::Active;
}

void UOutbreakCompositeObjective::Deactivate()
{
	// Sub-objectives still running stop with the composite (e.g. the other route once one succeeded).
	for (UOutbreakObjective* Sub : SubObjectives)
	{
		if (Sub && Sub->GetState() == EOutbreakObjectiveState::Active)
		{
			Sub->Deactivate();
			Sub->SetState(EOutbreakObjectiveState::Inactive);
		}
	}
}

bool UOutbreakCompositeObjective::Validate(TArray<FText>& OutErrors) const
{
	const int32 InitialErrorCount = OutErrors.Num();
	if (SubObjectives.IsEmpty())
	{
		OutErrors.Add(LOCTEXT("NoSubObjectives", "Composite objective has no sub-objectives."));
	}
	if (RequiredCount > SubObjectives.Num())
	{
		OutErrors.Add(FText::Format(LOCTEXT("RequiredCount", "Composite objective needs {0} of only {1} sub-objectives."),
			RequiredCount, SubObjectives.Num()));
	}
	for (int32 Index = 0; Index < SubObjectives.Num(); ++Index)
	{
		if (!SubObjectives[Index])
		{
			OutErrors.Add(FText::Format(LOCTEXT("NullSub", "Composite objective: sub-objective {0} is empty."), Index));
			continue;
		}
		SubObjectives[Index]->Validate(OutErrors);
	}
	return OutErrors.Num() == InitialErrorCount;
}

bool UOutbreakCompositeObjective::ValidateReferences(const TSet<FName>& ObjectiveIds, TArray<FText>& OutErrors) const
{
	bool bValid = true;
	for (const UOutbreakObjective* Sub : SubObjectives)
	{
		bValid &= !Sub || Sub->ValidateReferences(ObjectiveIds, OutErrors);
	}
	return bValid;
}

FText UOutbreakCompositeObjective::GetProgressText() const
{
	return FText::Format(LOCTEXT("Progress", "{0} / {1}"), GetCompletedCount(), GetNeededCount());
}

#undef LOCTEXT_NAMESPACE
