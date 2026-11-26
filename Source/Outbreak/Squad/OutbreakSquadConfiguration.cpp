#include "Squad/OutbreakSquadConfiguration.h"

#include "Squad/OutbreakTeammateCharacter.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "OutbreakSquadConfiguration"

bool UOutbreakSquadConfiguration::Validate(TArray<FText>& OutErrors) const
{
	const int32 InitialErrorCount = OutErrors.Num();

	if (SquadSize < 0 || MinSquadSizeOverride < 0 || MaxSquadSizeOverride < 0)
	{
		OutErrors.Add(LOCTEXT("Negative", "SquadSize and the size overrides must not be negative."));
	}
	if (MinSquadSizeOverride > 0 && MaxSquadSizeOverride > 0 && MinSquadSizeOverride > MaxSquadSizeOverride)
	{
		OutErrors.Add(LOCTEXT("MinAboveMax", "MinSquadSizeOverride is greater than MaxSquadSizeOverride."));
	}
	if (SquadSize > 0 && MinSquadSizeOverride > 0 && SquadSize < MinSquadSizeOverride)
	{
		OutErrors.Add(LOCTEXT("BelowMin", "SquadSize is below MinSquadSizeOverride."));
	}
	if (SquadSize > 0 && MaxSquadSizeOverride > 0 && SquadSize > MaxSquadSizeOverride)
	{
		OutErrors.Add(LOCTEXT("AboveMax", "SquadSize is above MaxSquadSizeOverride."));
	}

	return OutErrors.Num() == InitialErrorCount;
}

#if WITH_EDITOR
EDataValidationResult UOutbreakSquadConfiguration::IsDataValid(FDataValidationContext& Context) const
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
