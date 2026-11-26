#include "Inventory/OutbreakItemDefinition.h"

#include "Inventory/OutbreakItemAction.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "OutbreakItemDefinition"

bool UOutbreakItemDefinition::Validate(TArray<FText>& OutErrors) const
{
	const int32 InitialErrorCount = OutErrors.Num();

	if (Category == EOutbreakItemCategory::Weapon || Category == EOutbreakItemCategory::Ammunition)
	{
		OutErrors.Add(LOCTEXT("Category", "Weapons and ammunition are their own definitions; use Medical, Utility or Mission."));
	}
	if (Weight < 0.0f || MaxStack < 0)
	{
		OutErrors.Add(LOCTEXT("Negative", "Weight and MaxStack must not be negative."));
	}
	if ((Category == EOutbreakItemCategory::Medical || Category == EOutbreakItemCategory::Utility) && !UseAction)
	{
		OutErrors.Add(LOCTEXT("NoAction", "Medical and utility items need a UseAction."));
	}

	return OutErrors.Num() == InitialErrorCount;
}

#if WITH_EDITOR
EDataValidationResult UOutbreakItemDefinition::IsDataValid(FDataValidationContext& Context) const
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
