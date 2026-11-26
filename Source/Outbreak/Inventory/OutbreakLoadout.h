#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Weapons/OutbreakWeaponTypes.h"

#include "OutbreakLoadout.generated.h"

class UOutbreakWeaponDefinition;

/** A complete set of equipment for one soldier (Phase 10 equipment configuration). */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakLoadout
{
	GENERATED_BODY()

	/** Must use the Primary equipment slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	TObjectPtr<UOutbreakWeaponDefinition> PrimaryWeapon;

	/** Must use the Secondary equipment slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	TObjectPtr<UOutbreakWeaponDefinition> SecondaryWeapon;

	/** Reserve ammunition (magazines start full on top of this). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	TArray<FOutbreakAmmoAmount> Ammo;

	/** Medical, utility and mission items. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	TArray<FOutbreakItemAmount> Items;

	/** Carrying capacity in kilograms for whoever takes this loadout. 0 = keep the soldier's own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout", meta = (ClampMin = "0.0", Units = "kg"))
	float CarryCapacity = 0.0f;
};

namespace OutbreakLoadout
{
	/** Total weight of the loadout: weapons, reserve plus full magazines, items. */
	OUTBREAK_API float ComputeWeight(const FOutbreakLoadout& Loadout);

	/**
	 * Slots match, counts are positive, items are valid, and the weight fits the loadout's capacity
	 * (or FallbackCapacity when the loadout keeps the soldier's own; 0 = not checked).
	 */
	OUTBREAK_API bool Validate(const FOutbreakLoadout& Loadout, float FallbackCapacity, TArray<FText>& OutErrors);
}

/**
 * A named loadout (Rifleman kit, Medic kit, Breacher kit, ...) that roles, squad members and the
 * player can use, and that squad equipment selection chooses between.
 */
UCLASS(BlueprintType, Const)
class OUTBREAK_API UOutbreakLoadoutDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	bool Validate(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout", meta = (ShowOnlyInnerProperties))
	FOutbreakLoadout Loadout;
};
