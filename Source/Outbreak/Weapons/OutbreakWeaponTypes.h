#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"

#include "OutbreakWeaponTypes.generated.h"

class UOutbreakAmmoTypeDefinition;
class UOutbreakWeaponDefinition;

/** A quantity of one ammunition type (reserves, loadouts). */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakAmmoAmount
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammunition")
	TObjectPtr<UOutbreakAmmoTypeDefinition> AmmoType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammunition", meta = (ClampMin = "0"))
	int32 Count = 0;
};

/** Runtime state of one carried weapon. Replicated as part of UOutbreakWeaponComponent. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakWeaponSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UOutbreakWeaponDefinition> Definition;

	/** Ammunition currently in the magazine. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UOutbreakAmmoTypeDefinition> LoadedAmmoType;

	/** Ammunition the next reload loads. Differs from LoadedAmmoType after the player switches type. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UOutbreakAmmoTypeDefinition> SelectedAmmoType;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	int32 AmmoInMagazine = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	int32 FireModeIndex = 0;
};

/** Where one trace of a shot ended, for cosmetic effects on every machine. */
USTRUCT()
struct OUTBREAK_API FOutbreakShotImpact
{
	GENERATED_BODY()

	UPROPERTY()
	FVector_NetQuantize Location = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantizeNormal Normal = FVector::ZeroVector;

	/** False when the trace reached its range without hitting anything. */
	UPROPERTY()
	bool bBlockingHit = false;
};
