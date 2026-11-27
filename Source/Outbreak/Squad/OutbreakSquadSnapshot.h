#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"

#include "OutbreakSquadSnapshot.generated.h"

class AActor;
class UOutbreakAmmoTypeDefinition;
class UOutbreakItemDefinition;
class UOutbreakLoadoutDefinition;
class UOutbreakSquadComponent;
class UOutbreakWeaponDefinition;
struct FOutbreakDeploymentSelection;

/** One carried weapon in a checkpoint. Assets are soft references, so a save never pins or loads stale objects. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakSavedWeapon
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	TSoftObjectPtr<UOutbreakWeaponDefinition> Weapon;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	TSoftObjectPtr<UOutbreakAmmoTypeDefinition> LoadedAmmoType;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	TSoftObjectPtr<UOutbreakAmmoTypeDefinition> SelectedAmmoType;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	int32 AmmoInMagazine = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	int32 FireModeIndex = 0;
};

USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakSavedAmmo
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	TSoftObjectPtr<UOutbreakAmmoTypeDefinition> AmmoType;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	int32 Count = 0;
};

USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakSavedItem
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	TSoftObjectPtr<UOutbreakItemDefinition> Item;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	int32 Count = 0;
};

/** One soldier's state at a checkpoint: health, weapons, ammunition and items. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakSoldierSnapshot
{
	GENERATED_BODY()

	/** Dead soldiers stay dead after loading. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	bool bDead = false;

	/** Health / max health. A soldier down at the checkpoint is restored standing (see ApplySoldier). */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	float HealthFraction = 1.0f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	int32 ReviveCount = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	TArray<FOutbreakSavedWeapon> Weapons;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	int32 ActiveWeaponIndex = INDEX_NONE;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	TArray<FOutbreakSavedAmmo> AmmoReserves;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	TArray<FOutbreakSavedItem> Items;
};

/**
 * A squad at a checkpoint (Phase 14): the deployment selection it was built from, so loading
 * rebuilds the same soldiers, and each member's state. Members[0] is the player, then the teammate
 * slots in squad order.
 */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakSquadSnapshot
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	int32 SquadSize = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	TArray<int32> TeammateRosterIndices;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	TArray<TSoftObjectPtr<UOutbreakLoadoutDefinition>> MemberLoadouts;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	TArray<FOutbreakSoldierSnapshot> Members;

	bool IsEmpty() const { return SquadSize <= 0; }
};

/** Capturing and restoring squad state (authority). */
namespace OutbreakSquadSnapshot
{
	OUTBREAK_API FOutbreakSoldierSnapshot CaptureSoldier(const AActor& Soldier);

	/**
	 * Restores health, weapons, ammunition and items. Missing assets are skipped (logged). A soldier
	 * saved while down stands up at the medical settings' revive health; a dead one dies again.
	 */
	OUTBREAK_API void ApplySoldier(AActor& Soldier, const FOutbreakSoldierSnapshot& Snapshot);

	OUTBREAK_API FOutbreakSquadSnapshot CaptureSquad(const UOutbreakSquadComponent& Squad, const FOutbreakDeploymentSelection& Selection);

	/** The selection the snapshot was built from (loadouts loaded; missing ones become default equipment). */
	OUTBREAK_API FOutbreakDeploymentSelection ToSelection(const FOutbreakSquadSnapshot& Snapshot);

	/** Applies member states by squad order. Returns the number of members restored. */
	OUTBREAK_API int32 ApplySquad(UOutbreakSquadComponent& Squad, const FOutbreakSquadSnapshot& Snapshot);
}
