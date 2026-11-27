#include "Missions/OutbreakAvoidAlarmObjective.h"

#include "Engine/World.h"
#include "Missions/OutbreakAlarmSubsystem.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Outbreak.h"

#define LOCTEXT_NAMESPACE "OutbreakAvoidAlarmObjective"

EOutbreakObjectiveState UOutbreakAvoidAlarmObjective::Evaluate(UOutbreakMissionSubsystem& Mission)
{
	const UWorld* World = Mission.GetWorld();
	const UOutbreakAlarmSubsystem* Alarm = World ? World->GetSubsystem<UOutbreakAlarmSubsystem>() : nullptr;
	if (Alarm && Alarm->IsAlarmRaised())
	{
		return EOutbreakObjectiveState::Failed;
	}

	const UOutbreakObjective* Until = Mission.FindObjectiveById(UntilObjectiveId);
	if (!Until)
	{
		// Validation catches this in the editor; at runtime fail visibly instead of never finishing.
		UE_LOG(LogOutbreak, Error, TEXT("Avoid Alarm objective: no objective '%s'."), *UntilObjectiveId.ToString());
		return EOutbreakObjectiveState::Failed;
	}
	switch (Until->GetState())
	{
	case EOutbreakObjectiveState::Completed:
		return EOutbreakObjectiveState::Completed;
	case EOutbreakObjectiveState::Failed:
		return EOutbreakObjectiveState::Failed;
	default:
		return EOutbreakObjectiveState::Active;
	}
}

bool UOutbreakAvoidAlarmObjective::Validate(TArray<FText>& OutErrors) const
{
	if (UntilObjectiveId.IsNone())
	{
		OutErrors.Add(LOCTEXT("NoUntil", "Avoid Alarm objective has no Until Objective Id."));
		return false;
	}
	if (UntilObjectiveId == ObjectiveId)
	{
		OutErrors.Add(LOCTEXT("SelfUntil", "Avoid Alarm objective cannot wait for itself."));
		return false;
	}
	return true;
}

bool UOutbreakAvoidAlarmObjective::ValidateReferences(const TSet<FName>& ObjectiveIds, TArray<FText>& OutErrors) const
{
	if (!UntilObjectiveId.IsNone() && !ObjectiveIds.Contains(UntilObjectiveId))
	{
		OutErrors.Add(FText::Format(LOCTEXT("UnknownUntil", "Avoid Alarm objective waits for unknown objective '{0}'."),
			FText::FromName(UntilObjectiveId)));
		return false;
	}
	return true;
}

#undef LOCTEXT_NAMESPACE
