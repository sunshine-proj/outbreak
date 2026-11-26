#include "Inventory/OutbreakLoadout.h"

#include "Inventory/OutbreakItemDefinition.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakWeaponDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "OutbreakLoadout"

namespace OutbreakLoadout
{
	float ComputeWeight(const FOutbreakLoadout& Loadout)
	{
		float Weight = 0.0f;
		for (const UOutbreakWeaponDefinition* Weapon : { Loadout.PrimaryWeapon.Get(), Loadout.SecondaryWeapon.Get() })
		{
			if (Weapon)
			{
				Weight += Weapon->Weight;
				// A full magazine of the first ammunition type is loaded on top of the reserve.
				if (!Weapon->bUnlimitedAmmo && !Weapon->CompatibleAmmoTypes.IsEmpty() && Weapon->CompatibleAmmoTypes[0])
				{
					Weight += Weapon->CompatibleAmmoTypes[0]->WeightPerRound * Weapon->MagazineCapacity;
				}
			}
		}
		for (const FOutbreakAmmoAmount& Entry : Loadout.Ammo)
		{
			if (Entry.AmmoType)
			{
				Weight += Entry.AmmoType->WeightPerRound * FMath::Max(0, Entry.Count);
			}
		}
		for (const FOutbreakItemAmount& Entry : Loadout.Items)
		{
			if (Entry.Item)
			{
				Weight += Entry.Item->Weight * FMath::Max(0, Entry.Count);
			}
		}
		return Weight;
	}

	bool Validate(const FOutbreakLoadout& Loadout, float FallbackCapacity, TArray<FText>& OutErrors)
	{
		const int32 InitialErrorCount = OutErrors.Num();

		if (Loadout.PrimaryWeapon && Loadout.PrimaryWeapon->EquipmentSlot != EOutbreakEquipmentSlot::Primary)
		{
			OutErrors.Add(LOCTEXT("PrimarySlot", "PrimaryWeapon is not a primary-slot weapon."));
		}
		if (Loadout.SecondaryWeapon && Loadout.SecondaryWeapon->EquipmentSlot != EOutbreakEquipmentSlot::Secondary)
		{
			OutErrors.Add(LOCTEXT("SecondarySlot", "SecondaryWeapon is not a secondary-slot weapon."));
		}
		for (const FOutbreakAmmoAmount& Entry : Loadout.Ammo)
		{
			if (!Entry.AmmoType || Entry.Count <= 0)
			{
				OutErrors.Add(LOCTEXT("Ammo", "Every ammunition entry needs a type and a positive count."));
				break;
			}
		}
		for (const FOutbreakItemAmount& Entry : Loadout.Items)
		{
			if (!Entry.Item || Entry.Count <= 0)
			{
				OutErrors.Add(LOCTEXT("Items", "Every item entry needs an item and a positive count."));
				break;
			}
			if (Entry.Item->MaxStack > 0 && Entry.Count > Entry.Item->MaxStack)
			{
				OutErrors.Add(FText::Format(LOCTEXT("Stack", "{0}: {1} is above its MaxStack of {2}."),
					Entry.Item->DisplayName, Entry.Count, Entry.Item->MaxStack));
			}
		}

		const float Capacity = Loadout.CarryCapacity > 0.0f ? Loadout.CarryCapacity : FallbackCapacity;
		const float Weight = ComputeWeight(Loadout);
		if (Capacity > 0.0f && Weight > Capacity + 1.0e-3f)
		{
			OutErrors.Add(FText::Format(LOCTEXT("Weight", "Loadout weighs {0} kg, above the carrying capacity of {1} kg."),
				FText::AsNumber(Weight), FText::AsNumber(Capacity)));
		}

		return OutErrors.Num() == InitialErrorCount;
	}
}

bool UOutbreakLoadoutDefinition::Validate(TArray<FText>& OutErrors) const
{
	return OutbreakLoadout::Validate(Loadout, 0.0f, OutErrors);
}

#if WITH_EDITOR
EDataValidationResult UOutbreakLoadoutDefinition::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Result = Super::IsDataValid(Context);

	TArray<FText> Errors;
	if (!Validate(Errors))
	{
		for (const FText& Error : Errors)
		{
			Context.AddError(Error);
		}
		return EDataValidationResult::Invalid;
	}
	if (Loadout.CarryCapacity <= 0.0f)
	{
		Context.AddWarning(FText::Format(LOCTEXT("NoCapacity",
			"No CarryCapacity: the weight ({0} kg) is only checked against the soldier's capacity when it is applied."),
			FText::AsNumber(OutbreakLoadout::ComputeWeight(Loadout))));
	}
	return CombineDataValidationResults(Result, EDataValidationResult::Valid);
}
#endif

#undef LOCTEXT_NAMESPACE
