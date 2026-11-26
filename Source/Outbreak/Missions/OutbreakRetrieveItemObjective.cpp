#include "Missions/OutbreakRetrieveItemObjective.h"

#include "GameFramework/Actor.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Inventory/OutbreakItemDefinition.h"
#include "Missions/OutbreakMissionMarker.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Outbreak.h"

#define LOCTEXT_NAMESPACE "OutbreakObjectives"

void UOutbreakRetrieveItemObjective::Activate(UOutbreakMissionSubsystem& Mission)
{
	CarriedCount = 0;
	DeliveredCount = 0;
	DeliveryDistance = -1.0f;
	DeliveryMarker.Reset();

	if (!DeliveryMarkerId.IsNone())
	{
		DeliveryMarker = AOutbreakMissionMarker::FindMarker(Mission.GetWorld(), DeliveryMarkerId);
		if (!DeliveryMarker.IsValid())
		{
			UE_LOG(LogOutbreak, Error, TEXT("Retrieve Item objective: no AOutbreakMissionMarker with ID '%s' in the level; the objective fails."),
				*DeliveryMarkerId.ToString());
		}
	}
}

EOutbreakObjectiveState UOutbreakRetrieveItemObjective::Evaluate(UOutbreakMissionSubsystem& Mission)
{
	const bool bNeedsDelivery = !DeliveryMarkerId.IsNone();
	const AOutbreakMissionMarker* Destination = DeliveryMarker.Get();
	if (bNeedsDelivery && !Destination)
	{
		return EOutbreakObjectiveState::Failed;
	}

	CarriedCount = 0;
	DeliveredCount = 0;
	DeliveryDistance = -1.0f;
	for (const AActor* Soldier : Mission.GetSquadSoldiers())
	{
		const UOutbreakInventoryComponent* Inventory = Soldier->FindComponentByClass<UOutbreakInventoryComponent>();
		const int32 Carried = Inventory ? Inventory->GetItemCount(Item) : 0;
		if (Carried <= 0)
		{
			continue;
		}
		CarriedCount += Carried;

		if (Destination)
		{
			const float Distance = FVector::Dist(Soldier->GetActorLocation(), Destination->GetActorLocation());
			DeliveryDistance = DeliveryDistance < 0.0f ? Distance : FMath::Min(DeliveryDistance, Distance);
			DeliveredCount += Distance <= DeliveryRadius ? Carried : 0;
		}
	}

	const int32 Achieved = bNeedsDelivery ? DeliveredCount : CarriedCount;
	return Achieved >= Count ? EOutbreakObjectiveState::Completed : EOutbreakObjectiveState::Active;
}

void UOutbreakRetrieveItemObjective::Deactivate()
{
	DeliveryMarker.Reset();
}

bool UOutbreakRetrieveItemObjective::Validate(TArray<FText>& OutErrors) const
{
	if (!Item)
	{
		OutErrors.Add(LOCTEXT("RetrieveNoItem", "Retrieve Item objective has no Item."));
		return false;
	}
	return true;
}

FText UOutbreakRetrieveItemObjective::GetProgressText() const
{
	if (CarriedCount < Count || DeliveryMarkerId.IsNone())
	{
		return FText::Format(LOCTEXT("RetrieveProgress", "{0} / {1} recovered"), FMath::Min(CarriedCount, Count), Count);
	}
	return DeliveryDistance >= 0.0f
		? FText::Format(LOCTEXT("RetrieveDeliver", "deliver: {0} m"), FMath::RoundToInt(DeliveryDistance / 100.0f))
		: FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
