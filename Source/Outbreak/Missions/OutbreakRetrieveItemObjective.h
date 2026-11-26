#pragma once

#include "CoreMinimal.h"
#include "Missions/OutbreakObjective.h"

#include "OutbreakRetrieveItemObjective.generated.h"

class AOutbreakMissionMarker;
class UOutbreakItemDefinition;

/**
 * Recover Count of a mission item. Place an AOutbreakPickup holding Item in the level.
 *
 * Without a delivery marker it completes as soon as the squad (players and teammates) carries
 * Count. With one, the carried items must also be brought within DeliveryRadius of the marker.
 */
UCLASS(DisplayName = "Retrieve Item")
class OUTBREAK_API UOutbreakRetrieveItemObjective : public UOutbreakObjective
{
	GENERATED_BODY()

public:

	virtual void Activate(UOutbreakMissionSubsystem& Mission) override;
	virtual EOutbreakObjectiveState Evaluate(UOutbreakMissionSubsystem& Mission) override;
	virtual void Deactivate() override;
	virtual bool Validate(TArray<FText>& OutErrors) const override;
	virtual FText GetProgressText() const override;

	/** Usually a Mission-category item that cannot be dropped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	TObjectPtr<UOutbreakItemDefinition> Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "1"))
	int32 Count = 1;

	/** Optional AOutbreakMissionMarker::MarkerId to bring the item to. None = carrying it is enough. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName DeliveryMarkerId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "10.0", Units = "cm"))
	float DeliveryRadius = 300.0f;

	int32 GetCarriedCount() const { return CarriedCount; }

private:

	TWeakObjectPtr<AOutbreakMissionMarker> DeliveryMarker;
	int32 CarriedCount = 0;
	int32 DeliveredCount = 0;
	float DeliveryDistance = -1.0f;
};
