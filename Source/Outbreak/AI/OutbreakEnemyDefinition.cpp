#include "AI/OutbreakEnemyDefinition.h"

#include "BehaviorTree/BehaviorTree.h"
#include "Weapons/OutbreakWeaponDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "OutbreakEnemyDefinition"

bool UOutbreakEnemyDefinition::Validate(TArray<FText>& OutErrors) const
{
	const int32 InitialErrorCount = OutErrors.Num();

	if (MaxHealth < 1.0f)
	{
		OutErrors.Add(LOCTEXT("MaxHealth", "MaxHealth must be at least 1."));
	}
	if (LoseSightRadius < SightRadius)
	{
		OutErrors.Add(LOCTEXT("LoseSight", "LoseSightRadius must be at least SightRadius."));
	}
	if (!(MinCombatRange <= PreferredCombatRange && PreferredCombatRange <= MaxCombatRange))
	{
		OutErrors.Add(LOCTEXT("CombatRanges", "Combat ranges must satisfy Min <= Preferred <= Max."));
	}
	if (Weapon)
	{
		Weapon->Validate(OutErrors);
	}

	return OutErrors.Num() == InitialErrorCount;
}

#if WITH_EDITOR
EDataValidationResult UOutbreakEnemyDefinition::IsDataValid(FDataValidationContext& Context) const
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

	if (!Weapon)
	{
		Context.AddWarning(LOCTEXT("NoWeapon", "No Weapon: the enemy cannot attack."));
	}
	if (!BehaviorTree)
	{
		Context.AddWarning(LOCTEXT("NoTree", "No BehaviorTree: the enemy will not act."));
	}

	return CombineDataValidationResults(Result, EDataValidationResult::Valid);
}
#endif

#undef LOCTEXT_NAMESPACE
