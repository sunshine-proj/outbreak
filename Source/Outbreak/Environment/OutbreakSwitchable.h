#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "OutbreakSwitchable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UOutbreakSwitchable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Something interactable equipment can switch (Phase 12): a security door, an alarm, a Blueprint
 * machine. AOutbreakMissionEquipment calls it on its Linked Actors when it is switched on or off.
 * Actors without this interface that have light components get those lights switched instead.
 *
 * Called on the authority; implementers replicate their own state.
 */
class OUTBREAK_API IOutbreakSwitchable
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Environment")
	void SetSwitchedOn(bool bOn, AActor* SwitchedBy);

protected:

	virtual void SetSwitchedOn_Implementation(bool bOn, AActor* SwitchedBy) {}
};
