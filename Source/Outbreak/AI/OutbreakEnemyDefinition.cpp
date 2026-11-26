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
	if (!(SuspiciousThreshold <= InvestigateThreshold && InvestigateThreshold <= UrgentThreshold))
	{
		OutErrors.Add(LOCTEXT("Thresholds", "Detection thresholds must satisfy Suspicious <= Investigate <= Urgent."));
	}
	if (VisibilityWeights.DarkLightLevel >= VisibilityWeights.BrightLightLevel)
	{
		OutErrors.Add(LOCTEXT("LightLevels", "VisibilityWeights: DarkLightLevel must be below BrightLightLevel."));
	}
	if (!(MinCombatRange <= PreferredCombatRange && PreferredCombatRange <= MaxCombatRange))
	{
		OutErrors.Add(LOCTEXT("CombatRanges", "Combat ranges must satisfy Min <= Preferred <= Max."));
	}
	Tactics.Validate(OutErrors);
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
	if (Tactics.CombatStyle == EOutbreakEnemyCombatStyle::Melee && Weapon && Weapon->FireModes.Num() > 0
		&& Weapon->FireModes[0].Range < MaxCombatRange)
	{
		Context.AddWarning(LOCTEXT("MeleeReach", "Melee style: the weapon's Range is shorter than MaxCombatRange, so attacks may swing at air."));
	}

	return CombineDataValidationResults(Result, EDataValidationResult::Valid);
}
#endif

#undef LOCTEXT_NAMESPACE
