#include "Missions/OutbreakMissionDefinition.h"

#include "Missions/OutbreakObjective.h"
#include "Squad/OutbreakSquadConfiguration.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "OutbreakMissionDefinition"

bool UOutbreakMissionDefinition::Validate(TArray<FText>& OutErrors) const
{
	const int32 InitialErrorCount = OutErrors.Num();

	if (Objectives.IsEmpty())
	{
		OutErrors.Add(LOCTEXT("NoObjectives", "A mission needs at least one objective."));
	}
	for (int32 Index = 0; Index < Objectives.Num(); ++Index)
	{
		if (!Objectives[Index])
		{
			OutErrors.Add(FText::Format(LOCTEXT("NullObjective", "Objective {0} is empty."), Index));
		}
		else
		{
			Objectives[Index]->Validate(OutErrors);
		}
	}
	if (SquadConfiguration)
	{
		SquadConfiguration->Validate(OutErrors);
	}

	return OutErrors.Num() == InitialErrorCount;
}

#if WITH_EDITOR
EDataValidationResult UOutbreakMissionDefinition::IsDataValid(FDataValidationContext& Context) const
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
