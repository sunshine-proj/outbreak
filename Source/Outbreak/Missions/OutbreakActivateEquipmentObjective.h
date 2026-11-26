#pragma once

#include "CoreMinimal.h"
#include "Missions/OutbreakObjective.h"

#include "OutbreakActivateEquipmentObjective.generated.h"

class AOutbreakMissionEquipment;

/**
 * Switch on AOutbreakMissionEquipment actors tagged EquipmentTag. Unlocks them when the objective
 * becomes active and completes when RequiredCount of them (0 = all) are activated. Fails when
 * there is no tagged equipment, a level setup error.
 */
UCLASS(DisplayName = "Activate Equipment")
class OUTBREAK_API UOutbreakActivateEquipmentObjective : public UOutbreakObjective
{
	GENERATED_BODY()

public:

	virtual void Activate(UOutbreakMissionSubsystem& Mission) override;
	virtual EOutbreakObjectiveState Evaluate(UOutbreakMissionSubsystem& Mission) override;
	virtual void Deactivate() override;
	virtual bool Validate(TArray<FText>& OutErrors) const override;
	virtual FText GetProgressText() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName EquipmentTag;

	/** How many must be activated. 0 = every tagged piece of equipment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "0"))
	int32 RequiredCount = 0;

private:

	int32 GetRequired() const;

	TArray<TWeakObjectPtr<AOutbreakMissionEquipment>> Equipment;
	int32 ActivatedCount = 0;
};
