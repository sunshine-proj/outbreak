#include "Weapons/OutbreakWeaponTypes.h"

namespace OutbreakWeaponActions
{
	TArray<EOutbreakWeaponAction> DetectStateActions(bool bWasReloading, const UOutbreakWeaponDefinition* PreviousWeapon,
		bool bIsReloading, const UOutbreakWeaponDefinition* CurrentWeapon)
	{
		TArray<EOutbreakWeaponAction> Actions;
		// The first weapon given is not an "equip": soldiers spawn holding it.
		if (CurrentWeapon && PreviousWeapon && CurrentWeapon != PreviousWeapon)
		{
			Actions.Add(EOutbreakWeaponAction::Equipped);
		}
		if (bIsReloading && !bWasReloading)
		{
			Actions.Add(EOutbreakWeaponAction::ReloadStarted);
		}
		return Actions;
	}
}
