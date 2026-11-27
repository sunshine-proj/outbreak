#include "Missions/OutbreakMissionCheckpoint.h"

#include "Missions/OutbreakObjectiveGraph.h"

#define LOCTEXT_NAMESPACE "OutbreakMissionCheckpoint"

namespace OutbreakMissionCheckpointRules
{
	EOutbreakObjectiveState ToSavedState(EOutbreakObjectiveState State)
	{
		return State == EOutbreakObjectiveState::Completed || State == EOutbreakObjectiveState::Failed
			? State : EOutbreakObjectiveState::Inactive;
	}

	bool Validate(const FOutbreakMissionCheckpoint& Checkpoint, const TArray<FOutbreakObjectiveNode>& Nodes, TArray<FText>& OutErrors)
	{
		const int32 InitialErrorCount = OutErrors.Num();

		if (!Checkpoint.IsSet())
		{
			OutErrors.Add(LOCTEXT("NoMission", "The checkpoint names no mission."));
		}
		if (Checkpoint.ObjectiveStates.Num() != Nodes.Num())
		{
			OutErrors.Add(FText::Format(LOCTEXT("ObjectiveCount", "The checkpoint has {0} objective states; the mission has {1} objectives."),
				Checkpoint.ObjectiveStates.Num(), Nodes.Num()));
			return false;
		}
		if (!FMath::IsFinite(Checkpoint.MissionTime) || Checkpoint.MissionTime < 0.0f)
		{
			OutErrors.Add(LOCTEXT("MissionTime", "The checkpoint's mission time is invalid."));
		}

		TArray<TArray<int32>> Prerequisites;
		TArray<FText> GraphErrors;
		if (!OutbreakObjectiveGraph::Resolve(Nodes, Prerequisites, GraphErrors))
		{
			OutErrors.Append(GraphErrors);
			return false;
		}

		TArray<EOutbreakObjectiveState> States;
		for (const EOutbreakObjectiveState State : Checkpoint.ObjectiveStates)
		{
			States.Add(ToSavedState(State));
		}

		bool bRequiredLeft = false;
		for (int32 Index = 0; Index < Nodes.Num(); ++Index)
		{
			if (!Nodes[Index].bOptional)
			{
				if (States[Index] == EOutbreakObjectiveState::Failed)
				{
					OutErrors.Add(FText::Format(LOCTEXT("RequiredFailed", "Required objective {0} is failed in the checkpoint."), Index + 1));
				}
				bRequiredLeft |= States[Index] != EOutbreakObjectiveState::Completed;
			}
			if (States[Index] == EOutbreakObjectiveState::Completed && !OutbreakObjectiveGraph::ArePrerequisitesMet(Index, Prerequisites, States))
			{
				OutErrors.Add(FText::Format(LOCTEXT("OutOfOrder", "Objective {0} is completed before its prerequisites."), Index + 1));
			}
		}
		if (!bRequiredLeft)
		{
			OutErrors.Add(LOCTEXT("NothingLeft", "Every required objective is completed in the checkpoint; the mission was already won."));
		}

		return OutErrors.Num() == InitialErrorCount;
	}
}

#undef LOCTEXT_NAMESPACE
