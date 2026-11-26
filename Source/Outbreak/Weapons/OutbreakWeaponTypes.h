#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"

#include "OutbreakWeaponTypes.generated.h"

class UOutbreakAmmoTypeDefinition;
class UOutbreakWeaponDefinition;

/** Something a weapon did, for sound, animation and squad callouts (Phase 13). Fired on every machine. */
UENUM(BlueprintType)
enum class EOutbreakWeaponAction : uint8
{
	Fired,
	ReloadStarted,
	Equipped,
	/** Tried to fire with nothing to load (owning machine only). */
	DryFired
};

/** Which body pose set an Animation Blueprint uses for a weapon (Phase 13). */
UENUM(BlueprintType)
enum class EOutbreakWeaponPose : uint8
{
	Unarmed,
	Pistol,
	Rifle,
	Heavy,
	Melee
};

namespace OutbreakWeaponActions
{
	/**
	 * Actions implied by a replicated state change: a reload starting, a different weapon equipped
	 * (not the first one given). Shots arrive separately, through the shot multicast.
	 */
	OUTBREAK_API TArray<EOutbreakWeaponAction> DetectStateActions(bool bWasReloading, const UOutbreakWeaponDefinition* PreviousWeapon,
		bool bIsReloading, const UOutbreakWeaponDefinition* CurrentWeapon);
}

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
