#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/OutbreakItem.h"

#include "OutbreakAmmoTypeDefinition.generated.h"

class UDamageType;
class UNiagaraSystem;
class USoundBase;

/**
 * An ammunition type: standard, armour-piercing, explosive, energy, ...
 *
 * Weapons list the ammunition they accept in UOutbreakWeaponDefinition::CompatibleAmmoTypes.
 * The asset's primary asset ID serves as the ammunition ID. Carried rounds are an inventory item
 * (IOutbreakItem, Phase 10) stored in the weapon component's reserve.
 */
UCLASS(BlueprintType, Const)
class OUTBREAK_API UOutbreakAmmoTypeDefinition : public UPrimaryDataAsset, public IOutbreakItem
{
	GENERATED_BODY()

public:

	UOutbreakAmmoTypeDefinition();

	/** Returns false and appends a reason for each invalid setting. */
	bool Validate(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	//~ Begin IOutbreakItem interface
	virtual FText GetItemDisplayName() const override { return DisplayName; }
	virtual EOutbreakItemCategory GetItemCategory() const override { return EOutbreakItemCategory::Ammunition; }
	virtual float GetItemUnitWeight() const override { return WeightPerRound; }
	//~ End IOutbreakItem interface

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammunition")
	FText DisplayName;

	/** Carried weight of one round, in kilograms (Phase 10 carrying capacity). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammunition", meta = (ClampMin = "0.0", Units = "kg"))
	float WeightPerRound = 0.012f;

	/** Multiplies the fire mode's BaseDamage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<UDamageType> DamageType;

	/** 0 = point damage; > 0 = radial damage with falloff around the impact. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0", Units = "cm"))
	float ExplosionRadius = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<USoundBase> ImpactSound;
};
