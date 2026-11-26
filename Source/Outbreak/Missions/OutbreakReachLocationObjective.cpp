#include "Missions/OutbreakReachLocationObjective.h"

#include "GameFramework/Pawn.h"
#include "Missions/OutbreakMissionMarker.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Outbreak.h"

#define LOCTEXT_NAMESPACE "OutbreakObjectives"

void UOutbreakReachLocationObjective::Activate(UOutbreakMissionSubsystem& Mission)
{
	Marker = AOutbreakMissionMarker::FindMarker(Mission.GetWorld(), MarkerId);
	LastDistance = -1.0f;
	if (!Marker.IsValid())
	{
		UE_LOG(LogOutbreak, Error, TEXT("Reach Location objective: no AOutbreakMissionMarker with ID '%s' in the level; the objective fails."),
			*MarkerId.ToString());
	}
}

EOutbreakObjectiveState UOutbreakReachLocationObjective::Evaluate(UOutbreakMissionSubsystem& Mission)
{
	const AOutbreakMissionMarker* Destination = Marker.Get();
	if (!Destination)
	{
		// A missing marker is a level setup error; failing is visible, a silent soft-lock is not.
		return EOutbreakObjectiveState::Failed;
	}

	float Closest = TNumericLimits<float>::Max();
	for (const APawn* Player : Mission.GetLivingPlayerPawns())
	{
		Closest = FMath::Min(Closest, FVector::Dist(Player->GetActorLocation(), Destination->GetActorLocation()));
	}
	LastDistance = Closest < TNumericLimits<float>::Max() ? Closest : -1.0f;

	return LastDistance >= 0.0f && LastDistance <= Radius ? EOutbreakObjectiveState::Completed : EOutbreakObjectiveState::Active;
}

bool UOutbreakReachLocationObjective::Validate(TArray<FText>& OutErrors) const
{
	if (MarkerId.IsNone())
	{
		OutErrors.Add(LOCTEXT("ReachNoMarker", "Reach Location objective has no MarkerId."));
		return false;
	}
	return true;
}

FText UOutbreakReachLocationObjective::GetProgressText() const
{
	return LastDistance >= 0.0f
		? FText::Format(LOCTEXT("ReachDistance", "{0} m"), FMath::RoundToInt(LastDistance / 100.0f))
		: FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
