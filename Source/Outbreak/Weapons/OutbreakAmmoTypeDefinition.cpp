#include "Weapons/OutbreakAmmoTypeDefinition.h"

#include "Combat/OutbreakDamageTypes.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "OutbreakAmmoType"

UOutbreakAmmoTypeDefinition::UOutbreakAmmoTypeDefinition()
{
	DamageType = UOutbreakBallisticDamageType::StaticClass();
}

bool UOutbreakAmmoTypeDefinition::Validate(TArray<FText>& OutErrors) const
{
	const int32 InitialErrorCount = OutErrors.Num();

	if (DamageMultiplier < 0.0f)
	{
		OutErrors.Add(LOCTEXT("NegativeMultiplier", "DamageMultiplier must not be negative."));
	}
	if (ExplosionRadius < 0.0f)
	{
		OutErrors.Add(LOCTEXT("NegativeRadius", "ExplosionRadius must not be negative."));
	}

	return OutErrors.Num() == InitialErrorCount;
}

#if WITH_EDITOR
EDataValidationResult UOutbreakAmmoTypeDefinition::IsDataValid(FDataValidationContext& Context) const
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
