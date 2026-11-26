#pragma once

#include "CoreMinimal.h"

/** Firing and reloading rules for AI teammates, kept free of actors so they can be unit tested. */
namespace OutbreakTeammateCombat
{
	/** True when ViewDirection points within ToleranceDegrees of TargetLocation. */
	OUTBREAK_API bool IsAimedAt(const FVector& ViewLocation, const FVector& ViewDirection, const FVector& TargetLocation, float ToleranceDegrees);

	/** Fire when the target is visible, in range, aimed at, and the weapon can fire. */
	OUTBREAK_API bool ShouldFire(bool bTargetVisible, float DistanceToTarget, float MaxRange, bool bAimed, bool bWeaponCanFire);

	/**
	 * Reload when the magazine is empty, or when no target is present and the magazine is below
	 * IdleReloadFraction of capacity. Never when the weapon cannot reload.
	 */
	OUTBREAK_API bool ShouldReload(int32 AmmoInMagazine, int32 MagazineCapacity, bool bHasTarget, float IdleReloadFraction, bool bWeaponCanReload);
}
