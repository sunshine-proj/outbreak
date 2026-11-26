#pragma once

#include "CoreMinimal.h"

class UObject;
class UOutbreakAmmoTypeDefinition;
class UOutbreakWeaponDefinition;
enum class EOutbreakWeaponAction : uint8;

/**
 * Cosmetic weapon effects (Niagara and sound). They never affect gameplay, do nothing on a
 * dedicated server, and do nothing when an asset is not assigned.
 */
namespace OutbreakWeaponEffects
{
	/** Muzzle flash and fire sound at the weapon's muzzle offset from the view point. */
	OUTBREAK_API void PlayMuzzleEffects(const UObject* WorldContextObject, const UOutbreakWeaponDefinition* Weapon,
		const FVector& ViewOrigin, const FVector& ViewDirection);

	/** Reload, equip or dry-fire sound of the weapon at Location (Fired uses PlayMuzzleEffects). */
	OUTBREAK_API void PlayActionSound(const UObject* WorldContextObject, const UOutbreakWeaponDefinition* Weapon,
		EOutbreakWeaponAction Action, const FVector& Location);

	/** Impact effect and sound of the ammunition type. */
	OUTBREAK_API void PlayImpactEffects(const UObject* WorldContextObject, const UOutbreakAmmoTypeDefinition* AmmoType,
		const FVector& Location, const FVector& Normal);
}
