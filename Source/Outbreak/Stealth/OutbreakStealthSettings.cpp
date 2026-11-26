#include "Stealth/OutbreakStealthSettings.h"

float UOutbreakStealthSettings::GetBaseNoiseRange(EOutbreakNoiseType Type) const
{
	switch (Type)
	{
	case EOutbreakNoiseType::Footstep:  return FootstepNoiseRange;
	case EOutbreakNoiseType::Weapon:    return WeaponNoiseRange;
	case EOutbreakNoiseType::Explosion: return ExplosionNoiseRange;
	case EOutbreakNoiseType::Impact:    return ImpactNoiseRange;
	default:                            return OtherNoiseRange;
	}
}
