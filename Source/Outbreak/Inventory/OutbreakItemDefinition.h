#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/OutbreakItem.h"

#include "OutbreakItemDefinition.generated.h"

class UOutbreakItemAction;
class UTexture2D;

/**
 * A carryable item that is neither a weapon nor ammunition (Phase 10): medkits, grenades and other
 * utility equipment, mission objects. What using it does is the instanced UseAction.
 * Weapons and ammunition are their own definitions and implement IOutbreakItem directly.
 */
UCLASS(BlueprintType, Const)
class OUTBREAK_API UOutbreakItemDefinition : public UPrimaryDataAsset, public IOutbreakItem
{
	GENERATED_BODY()

public:

	//~ Begin IOutbreakItem interface
	virtual FText GetItemDisplayName() const override { return DisplayName; }
	virtual EOutbreakItemCategory GetItemCategory() const override { return Category; }
	virtual float GetItemUnitWeight() const override { return Weight; }
	virtual int32 GetItemMaxStack() const override { return MaxStack; }
	//~ End IOutbreakItem interface

	/** Returns false and appends a reason for each invalid setting. */
	bool Validate(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (MultiLine = "true"))
	FText Description;

	/** For the inventory UI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Medical, Utility or Mission. (Weapon and Ammunition use their own definitions.) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EOutbreakItemCategory Category = EOutbreakItemCategory::Medical;

	/** Weight of one unit, in kilograms. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0.0", Units = "kg"))
	float Weight = 0.5f;

	/** Most units one soldier carries (a medical resource limit). 0 = only weight limits it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0"))
	int32 MaxStack = 3;

	/** Can be dropped (mission items usually cannot). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	bool bCanDrop = true;

	/** What using the item does. Empty = not usable (mission items). */
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Use")
	TObjectPtr<UOutbreakItemAction> UseAction;
};
