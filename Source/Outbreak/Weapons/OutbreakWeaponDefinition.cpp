#include "Weapons/OutbreakWeaponDefinition.h"

#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakFireBehavior.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "OutbreakWeaponDefinition"

float FOutbreakSpreadSettings::GetSpread(bool bAiming, float Bloom) const
{
	const float BaseSpread = bAiming ? AimSpread : HipSpread;
	return FMath::Clamp(BaseSpread + FMath::Max(0.0f, Bloom), 0.0f, FMath::Max(MaxSpread, BaseSpread));
}

float FOutbreakSpreadSettings::RecoverBloom(float Bloom, float ElapsedSeconds) const
{
	return FMath::Max(0.0f, Bloom - RecoveryPerSecond * FMath::Max(0.0f, ElapsedSeconds));
}

bool UOutbreakWeaponDefinition::IsAmmoTypeCompatible(const UOutbreakAmmoTypeDefinition* AmmoType) const
{
	return AmmoType && CompatibleAmmoTypes.ContainsByPredicate(
		[AmmoType](const TObjectPtr<UOutbreakAmmoTypeDefinition>& Entry) { return Entry.Get() == AmmoType; });
}

const FOutbreakFireModeDefinition* UOutbreakWeaponDefinition::GetFireMode(int32 FireModeIndex) const
{
	return FireModes.IsValidIndex(FireModeIndex) ? &FireModes[FireModeIndex] : nullptr;
}

bool UOutbreakWeaponDefinition::Validate(TArray<FText>& OutErrors) const
{
	const int32 InitialErrorCount = OutErrors.Num();

	if (MagazineCapacity < 1)
	{
		OutErrors.Add(LOCTEXT("MagazineCapacity", "MagazineCapacity must be at least 1."));
	}
	if (ReloadDuration < 0.0f)
	{
		OutErrors.Add(LOCTEXT("ReloadDuration", "ReloadDuration must not be negative."));
	}

	if (FireModes.IsEmpty())
	{
		OutErrors.Add(LOCTEXT("NoFireModes", "At least one fire mode is required."));
	}
	for (int32 Index = 0; Index < FireModes.Num(); ++Index)
	{
		const FOutbreakFireModeDefinition& Mode = FireModes[Index];
		if (!Mode.FireBehavior)
		{
			OutErrors.Add(FText::Format(LOCTEXT("NoBehavior", "Fire mode {0} has no FireBehavior."), Index));
		}
		if (Mode.RoundsPerMinute <= 0.0f)
		{
			OutErrors.Add(FText::Format(LOCTEXT("RoundsPerMinute", "Fire mode {0}: RoundsPerMinute must be positive."), Index));
		}
		if (Mode.ProjectilesPerShot < 1)
		{
			OutErrors.Add(FText::Format(LOCTEXT("Projectiles", "Fire mode {0}: ProjectilesPerShot must be at least 1."), Index));
		}
		if (Mode.AmmoPerShot < 1 || Mode.AmmoPerShot > MagazineCapacity)
		{
			OutErrors.Add(FText::Format(LOCTEXT("AmmoPerShot", "Fire mode {0}: AmmoPerShot must be between 1 and MagazineCapacity."), Index));
		}
		if (Mode.Range <= 0.0f)
		{
			OutErrors.Add(FText::Format(LOCTEXT("Range", "Fire mode {0}: Range must be positive."), Index));
		}
		if (Mode.BaseDamage < 0.0f)
		{
			OutErrors.Add(FText::Format(LOCTEXT("Damage", "Fire mode {0}: BaseDamage must not be negative."), Index));
		}
	}

	if (CompatibleAmmoTypes.IsEmpty())
	{
		OutErrors.Add(LOCTEXT("NoAmmo", "At least one compatible ammunition type is required."));
	}
	for (int32 Index = 0; Index < CompatibleAmmoTypes.Num(); ++Index)
	{
		if (!CompatibleAmmoTypes[Index])
		{
			OutErrors.Add(FText::Format(LOCTEXT("NullAmmo", "CompatibleAmmoTypes[{0}] is empty."), Index));
		}
		else
		{
			CompatibleAmmoTypes[Index]->Validate(OutErrors);
		}
	}

	return OutErrors.Num() == InitialErrorCount;
}

#if WITH_EDITOR
EDataValidationResult UOutbreakWeaponDefinition::IsDataValid(FDataValidationContext& Context) const
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

	return CombineDataValidationResults(Result, EDataValidationResult::Valid);
}
#endif

#undef LOCTEXT_NAMESPACE
