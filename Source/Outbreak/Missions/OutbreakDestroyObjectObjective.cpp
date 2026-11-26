#include "Missions/OutbreakDestroyObjectObjective.h"

#define LOCTEXT_NAMESPACE "OutbreakObjectives"

bool UOutbreakDestroyObjectObjective::Validate(TArray<FText>& OutErrors) const
{
	if (TargetTag.IsNone())
	{
		OutErrors.Add(LOCTEXT("DestroyNoTag", "Destroy Object objective has no TargetTag."));
		return false;
	}
	return true;
}

#undef LOCTEXT_NAMESPACE
