#include "AI/OutbreakAICombatRules.h"

namespace OutbreakAICombat
{
	bool IsAimedAt(const FVector& ViewLocation, const FVector& ViewDirection, const FVector& TargetLocation, float ToleranceDegrees)
	{
		const FVector ToTarget = (TargetLocation - ViewLocation).GetSafeNormal();
		const FVector View = ViewDirection.GetSafeNormal();
		if (ToTarget.IsNearlyZero() || View.IsNearlyZero())
		{
			return false;
		}
		return FVector::DotProduct(ToTarget, View) >= FMath::Cos(FMath::DegreesToRadians(ToleranceDegrees));
	}

	bool ShouldFire(bool bTargetVisible, float DistanceToTarget, float MaxRange, bool bAimed, bool bWeaponCanFire)
	{
		return bTargetVisible && bAimed && bWeaponCanFire && DistanceToTarget <= MaxRange;
	}

	bool ShouldReload(int32 AmmoInMagazine, int32 MagazineCapacity, bool bHasTarget, float IdleReloadFraction, bool bWeaponCanReload)
	{
		if (!bWeaponCanReload || MagazineCapacity <= 0)
		{
			return false;
		}
		if (AmmoInMagazine <= 0)
		{
			return true;
		}
		return !bHasTarget && AmmoInMagazine < FMath::CeilToInt(MagazineCapacity * IdleReloadFraction);
	}
}
