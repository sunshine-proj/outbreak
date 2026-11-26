#include "Missions/OutbreakObjectiveGraph.h"

#include "Missions/OutbreakObjective.h"

#define LOCTEXT_NAMESPACE "OutbreakObjectiveGraph"

FOutbreakObjectiveNode FOutbreakObjectiveNode::FromObjective(const UOutbreakObjective& Objective)
{
	FOutbreakObjectiveNode Node;
	Node.Id = Objective.GetObjectiveId();
	Node.Activation = Objective.GetActivation();
	Node.Prerequisites = Objective.GetPrerequisites();
	Node.bOptional = Objective.IsOptional();
	return Node;
}

namespace OutbreakObjectiveGraph
{
	namespace
	{
		FText Name(const TArray<FOutbreakObjectiveNode>& Nodes, int32 Index)
		{
			return Nodes[Index].Id.IsNone()
				? FText::Format(LOCTEXT("UnnamedObjective", "objective {0}"), Index)
				: FText::FromName(Nodes[Index].Id);
		}

		enum class EVisit : uint8 { New, InProgress, Done };

		bool HasCycleFrom(int32 Index, const TArray<TArray<int32>>& Prerequisites, TArray<EVisit>& Visits)
		{
			Visits[Index] = EVisit::InProgress;
			for (const int32 Prerequisite : Prerequisites[Index])
			{
				if (Visits[Prerequisite] == EVisit::InProgress
					|| (Visits[Prerequisite] == EVisit::New && HasCycleFrom(Prerequisite, Prerequisites, Visits)))
				{
					return true;
				}
			}
			Visits[Index] = EVisit::Done;
			return false;
		}
	}

	bool Resolve(const TArray<FOutbreakObjectiveNode>& Nodes, TArray<TArray<int32>>& OutPrerequisites, TArray<FText>& OutErrors)
	{
		const int32 InitialErrorCount = OutErrors.Num();
		OutPrerequisites.Reset();
		OutPrerequisites.SetNum(Nodes.Num());

		TMap<FName, int32> IndexById;
		bool bAnyRequired = false;
		for (int32 Index = 0; Index < Nodes.Num(); ++Index)
		{
			bAnyRequired |= !Nodes[Index].bOptional;
			if (Nodes[Index].Id.IsNone())
			{
				continue;
			}
			if (IndexById.Contains(Nodes[Index].Id))
			{
				OutErrors.Add(FText::Format(LOCTEXT("DuplicateId", "Objective ID '{0}' is used more than once."), FText::FromName(Nodes[Index].Id)));
				continue;
			}
			IndexById.Add(Nodes[Index].Id, Index);
		}
		if (!Nodes.IsEmpty() && !bAnyRequired)
		{
			OutErrors.Add(LOCTEXT("NoRequired", "A mission needs at least one required (non-optional) objective."));
		}

		for (int32 Index = 0; Index < Nodes.Num(); ++Index)
		{
			const FOutbreakObjectiveNode& Node = Nodes[Index];
			switch (Node.Activation)
			{
			case EOutbreakObjectiveActivation::AfterPrevious:
				for (int32 Previous = Index - 1; Previous >= 0; --Previous)
				{
					if (!Nodes[Previous].bOptional)
					{
						OutPrerequisites[Index].Add(Previous);
						break;
					}
				}
				break;

			case EOutbreakObjectiveActivation::AfterPrerequisites:
				if (Node.Prerequisites.IsEmpty())
				{
					OutErrors.Add(FText::Format(LOCTEXT("NoPrerequisites", "{0} activates after prerequisites but lists none."), Name(Nodes, Index)));
				}
				for (const FName& PrerequisiteId : Node.Prerequisites)
				{
					const int32* Found = IndexById.Find(PrerequisiteId);
					if (!Found)
					{
						OutErrors.Add(FText::Format(LOCTEXT("UnknownPrerequisite", "{0} depends on unknown objective '{1}'."),
							Name(Nodes, Index), FText::FromName(PrerequisiteId)));
					}
					else if (*Found == Index)
					{
						OutErrors.Add(FText::Format(LOCTEXT("SelfPrerequisite", "{0} depends on itself."), Name(Nodes, Index)));
					}
					else
					{
						OutPrerequisites[Index].AddUnique(*Found);
					}
				}
				break;

			default:
				break;
			}

			for (const int32 Prerequisite : OutPrerequisites[Index])
			{
				if (!Node.bOptional && Nodes[Prerequisite].bOptional)
				{
					OutErrors.Add(FText::Format(LOCTEXT("RequiredOnOptional", "Required {0} cannot depend on optional {1}."),
						Name(Nodes, Index), Name(Nodes, Prerequisite)));
				}
			}
		}

		TArray<EVisit> Visits;
		Visits.Init(EVisit::New, Nodes.Num());
		for (int32 Index = 0; Index < Nodes.Num(); ++Index)
		{
			if (Visits[Index] == EVisit::New && HasCycleFrom(Index, OutPrerequisites, Visits))
			{
				OutErrors.Add(LOCTEXT("Cycle", "Objective prerequisites form a cycle; those objectives could never activate."));
				break;
			}
		}

		return OutErrors.Num() == InitialErrorCount;
	}

	bool ArePrerequisitesMet(int32 Index, const TArray<TArray<int32>>& Prerequisites, const TArray<EOutbreakObjectiveState>& States)
	{
		if (!Prerequisites.IsValidIndex(Index))
		{
			return false;
		}
		for (const int32 Prerequisite : Prerequisites[Index])
		{
			if (!States.IsValidIndex(Prerequisite) || States[Prerequisite] != EOutbreakObjectiveState::Completed)
			{
				return false;
			}
		}
		return true;
	}

	bool IsBlocked(int32 Index, const TArray<TArray<int32>>& Prerequisites, const TArray<EOutbreakObjectiveState>& States)
	{
		if (!Prerequisites.IsValidIndex(Index))
		{
			return false;
		}
		// Resolve() rejects cycles, so the recursion terminates.
		for (const int32 Prerequisite : Prerequisites[Index])
		{
			if (States.IsValidIndex(Prerequisite)
				&& (States[Prerequisite] == EOutbreakObjectiveState::Failed || IsBlocked(Prerequisite, Prerequisites, States)))
			{
				return true;
			}
		}
		return false;
	}
}

#undef LOCTEXT_NAMESPACE
