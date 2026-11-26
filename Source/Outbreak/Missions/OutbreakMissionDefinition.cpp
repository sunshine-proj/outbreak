#include "Missions/OutbreakMissionDefinition.h"

#include "Missions/OutbreakObjective.h"
#include "Missions/OutbreakObjectiveGraph.h"
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

	bool bAllObjectivesSet = true;
	TArray<FOutbreakObjectiveNode> Nodes;
	for (int32 Index = 0; Index < Objectives.Num(); ++Index)
	{
		if (!Objectives[Index])
		{
			OutErrors.Add(FText::Format(LOCTEXT("NullObjective", "Objective {0} is empty."), Index));
			bAllObjectivesSet = false;
			continue;
		}
		Objectives[Index]->Validate(OutErrors);
		Nodes.Add(FOutbreakObjectiveNode::FromObjective(*Objectives[Index]));
	}

	// Indices only line up when every entry is set; an empty entry is already an error.
	if (bAllObjectivesSet && !Nodes.IsEmpty())
	{
		TArray<TArray<int32>> Prerequisites;
		OutbreakObjectiveGraph::Resolve(Nodes, Prerequisites, OutErrors);
	}

	if (SquadConfiguration)
	{
		SquadConfiguration->Validate(OutErrors);
	}

	return OutErrors.Num() == InitialErrorCount;
}

FName UOutbreakMissionDefinition::GetResolvedMissionId() const
{
	return MissionId.IsNone() ? GetFName() : MissionId;
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
