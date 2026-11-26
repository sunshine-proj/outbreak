#include "Missions/OutbreakProtectLocationObjective.h"

#include "Missions/OutbreakMissionMarker.h"
#include "Missions/OutbreakObjectiveQueries.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Outbreak.h"

#define LOCTEXT_NAMESPACE "OutbreakObjectives"

void UOutbreakProtectLocationObjective::Activate(UOutbreakMissionSubsystem& Mission)
{
	Marker = AOutbreakMissionMarker::FindMarker(Mission.GetWorld(), MarkerId);
	StartTime = Mission.GetMissionTime();
	LastEvaluationTime = StartTime;
	HostileTime = 0.0f;
	TimeRemaining = Duration;
	bContested = false;

	ProtectedActors.Reset();
	if (!ProtectedTag.IsNone())
	{
		for (AActor* Actor : OutbreakObjectiveQueries::FindActorsWithTag(Mission.GetWorld(), ProtectedTag))
		{
			ProtectedActors.Add(Actor);
		}
		if (ProtectedActors.IsEmpty())
		{
			UE_LOG(LogOutbreak, Warning, TEXT("Protect Location objective: no actors tagged '%s'; only the zone is defended."),
				*ProtectedTag.ToString());
		}
	}

	if (!Marker.IsValid())
	{
		UE_LOG(LogOutbreak, Error, TEXT("Protect Location objective: no AOutbreakMissionMarker with ID '%s' in the level; the objective fails."),
			*MarkerId.ToString());
	}
}

EOutbreakObjectiveState UOutbreakProtectLocationObjective::Evaluate(UOutbreakMissionSubsystem& Mission)
{
	const AOutbreakMissionMarker* Zone = Marker.Get();
	if (!Zone)
	{
		return EOutbreakObjectiveState::Failed;
	}

	for (const TWeakObjectPtr<AActor>& Protected : ProtectedActors)
	{
		if (OutbreakObjectiveQueries::IsDestroyed(Protected.Get()))
		{
			return EOutbreakObjectiveState::Failed;
		}
	}

	const float Now = Mission.GetMissionTime();
	const float DeltaTime = FMath::Max(0.0f, Now - LastEvaluationTime);
	LastEvaluationTime = Now;

	bContested = OutbreakObjectiveQueries::CountHostilesNear(Mission.GetWorld(), Zone->GetActorLocation(), Radius) > 0;
	if (bContested)
	{
		HostileTime += DeltaTime;
		if (MaxHostileTime <= 0.0f || HostileTime >= MaxHostileTime)
		{
			return EOutbreakObjectiveState::Failed;
		}
	}
	else
	{
		HostileTime = 0.0f;
	}

	TimeRemaining = FMath::Max(0.0f, Duration - (Now - StartTime));
	return TimeRemaining <= 0.0f ? EOutbreakObjectiveState::Completed : EOutbreakObjectiveState::Active;
}

void UOutbreakProtectLocationObjective::Deactivate()
{
	Marker.Reset();
	ProtectedActors.Reset();
	bContested = false;
}

bool UOutbreakProtectLocationObjective::Validate(TArray<FText>& OutErrors) const
{
	if (MarkerId.IsNone())
	{
		OutErrors.Add(LOCTEXT("ProtectNoMarker", "Protect Location objective has no MarkerId."));
		return false;
	}
	return true;
}

FText UOutbreakProtectLocationObjective::GetProgressText() const
{
	const FText Time = OutbreakObjectiveQueries::FormatTime(TimeRemaining);
	return bContested
		? FText::Format(LOCTEXT("ProtectContested", "{0} left, CONTESTED"), Time)
		: FText::Format(LOCTEXT("ProtectTime", "{0} left"), Time);
}

#undef LOCTEXT_NAMESPACE
