#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "OutbreakAmmoTypeDefinition.generated.h"

class UDamageType;
class UNiagaraSystem;
class USoundBase;

/**
 * An ammunition type: standard, armour-piercing, explosive, energy, ...
 *
 * Weapons list the ammunition they accept in UOutbreakWeaponDefinition::CompatibleAmmoTypes.
 * The asset's primary asset ID serves as the ammunition ID.
 */
UCLASS(BlueprintType, Const)
class OUTBREAK_API UOutbreakAmmoTypeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UOutbreakAmmoTypeDefinition();

	/** Returns false and appends a reason for each invalid setting. */
	bool Validate(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammunition")
	FText DisplayName;

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
