#pragma once

#include "CoreMinimal.h"
#include "Missions/OutbreakMissionTypes.h"

class UOutbreakObjective;

/** The dependency settings of one objective, separate from the UObject so the rules are testable. */
struct OUTBREAK_API FOutbreakObjectiveNode
{
	FName Id;
	EOutbreakObjectiveActivation Activation = EOutbreakObjectiveActivation::AfterPrevious;
	TArray<FName> Prerequisites;
	bool bOptional = false;

	static FOutbreakObjectiveNode FromObjective(const UOutbreakObjective& Objective);
};

/**
 * Objective dependency rules (Phase 11).
 *
 * - AfterPrevious depends on the nearest earlier required objective, so optional objectives listed
 *   in between run alongside the main line instead of blocking it.
 * - AfterPrerequisites depends on the listed IDs.
 * - A required objective may only depend on required objectives, so it can never be blocked by an
 *   optional failure.
 * - An objective whose prerequisite failed (or is blocked) can never activate.
 */
namespace OutbreakObjectiveGraph
{
	/**
	 * Resolves each node's prerequisites to indices. Fails, appending a reason for each problem,
	 * on duplicate IDs, unknown or self prerequisites, AfterPrerequisites without prerequisites,
	 * required-on-optional dependencies, cycles, or no required objective.
	 */
	OUTBREAK_API bool Resolve(const TArray<FOutbreakObjectiveNode>& Nodes, TArray<TArray<int32>>& OutPrerequisites, TArray<FText>& OutErrors);

	/** Every prerequisite of Index has completed. */
	OUTBREAK_API bool ArePrerequisitesMet(int32 Index, const TArray<TArray<int32>>& Prerequisites, const TArray<EOutbreakObjectiveState>& States);

	/** A prerequisite of Index has failed, directly or through its own prerequisites, so Index can never activate. */
	OUTBREAK_API bool IsBlocked(int32 Index, const TArray<TArray<int32>>& Prerequisites, const TArray<EOutbreakObjectiveState>& States);
}
