#include "AI/OutbreakTeammateAIConfig.h"

#include "BehaviorTree/BehaviorTree.h"
#include "Outbreak.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "OutbreakTeammateAIConfig"

UOutbreakTeammateAIConfig::UOutbreakTeammateAIConfig()
{
	PriorityOrder = OutbreakTeammateBehavior::GetDefaultPriorityOrder();
}

TArray<EOutbreakBehaviorLayer> UOutbreakTeammateAIConfig::GetEffectivePriorityOrder() const
{
	if (OutbreakTeammateBehavior::IsValidPriorityOrder(PriorityOrder))
	{
		return PriorityOrder;
	}

	static TSet<FName> WarnedAssets;
	if (!WarnedAssets.Contains(GetFName()))
	{
		WarnedAssets.Add(GetFName());
		UE_LOG(LogOutbreak, Warning, TEXT("%s: PriorityOrder must list every layer exactly once; using the default order."), *GetName());
	}
	return OutbreakTeammateBehavior::GetDefaultPriorityOrder();
}

bool UOutbreakTeammateAIConfig::Validate(TArray<FText>& OutErrors) const
{
	const int32 InitialErrorCount = OutErrors.Num();

	if (!OutbreakTeammateBehavior::IsValidPriorityOrder(PriorityOrder))
	{
		OutErrors.Add(LOCTEXT("PriorityOrder", "PriorityOrder must list every behaviour layer exactly once."));
	}
	if (LoseSightRadius < SightRadius)
	{
		OutErrors.Add(LOCTEXT("LoseSight", "LoseSightRadius must be at least SightRadius."));
	}
	if (FollowStopDistance >= FollowStartDistance)
	{
		OutErrors.Add(LOCTEXT("FollowHysteresis", "FollowStopDistance must be less than FollowStartDistance."));
	}

	return OutErrors.Num() == InitialErrorCount;
}

#if WITH_EDITOR
EDataValidationResult UOutbreakTeammateAIConfig::IsDataValid(FDataValidationContext& Context) const
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

	if (!BehaviorTree)
	{
		Context.AddWarning(LOCTEXT("NoTree", "No BehaviorTree assigned: teammates using this config will not act."));
	}

	return CombineDataValidationResults(Result, EDataValidationResult::Valid);
}
#endif

#undef LOCTEXT_NAMESPACE
