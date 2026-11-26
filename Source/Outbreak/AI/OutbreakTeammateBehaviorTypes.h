#pragma once

#include "CoreMinimal.h"
#include "Squad/OutbreakSquadCommandTypes.h"

#include "OutbreakTeammateBehaviorTypes.generated.h"

/** What a teammate is doing. Selected by the priority rules below; executed by the Behavior Tree. */
UENUM(BlueprintType)
enum class EOutbreakTeammateBehavior : uint8
{
	Idle,
	FollowPlayer,
	HoldPosition,
	MoveToLocation,
	Regroup,
	AttackTarget,
	EngageThreat,
	/** Low health under threat: fall back to the player. */
	SeekSafety
};

/**
 * Decision layers, evaluated in a configurable priority order (AGENTS.md §6.1). The first layer
 * that applies decides the behaviour.
 */
UENUM(BlueprintType)
enum class EOutbreakBehaviorLayer : uint8
{
	/** Low health while a threat is present. */
	EmergencySurvival,
	/** An active player order. */
	ExplicitOrder,
	/** A visible hostile. */
	ThreatResponse,
	/** Too far from the follow position. */
	FormationMaintenance,
	/** Nothing else applies. */
	DefaultAutonomous
};

/** Inputs to the behaviour decision. Gathered by the AI controller. */
struct FOutbreakBehaviorContext
{
	/** The member's order, or None if it has no active order. */
	EOutbreakSquadCommandType ActiveCommand = EOutbreakSquadCommandType::None;

	bool bHasThreat = false;
	bool bLowHealth = false;
	bool bHasPlayer = false;

	/** Distance from the member to its follow position near the player. */
	float DistanceToFollowLocation = 0.0f;

	/** Whether the member was following at the last decision (for hysteresis). */
	bool bCurrentlyFollowing = false;

	/** Start following when farther than this from the follow position. */
	float FollowStartDistance = 250.0f;

	/** Keep following until closer than this. */
	float FollowStopDistance = 100.0f;
};

struct FOutbreakBehaviorDecision
{
	EOutbreakTeammateBehavior Behavior = EOutbreakTeammateBehavior::Idle;
	EOutbreakBehaviorLayer Layer = EOutbreakBehaviorLayer::DefaultAutonomous;
};

namespace OutbreakTeammateBehavior
{
	/** Emergency survival > explicit order > threat response > formation > default (AGENTS.md §6.1). */
	OUTBREAK_API TArray<EOutbreakBehaviorLayer> GetDefaultPriorityOrder();

	/** True when every layer appears exactly once. */
	OUTBREAK_API bool IsValidPriorityOrder(const TArray<EOutbreakBehaviorLayer>& PriorityOrder);

	/** Whether Layer applies in Context, and the behaviour it selects. */
	OUTBREAK_API bool EvaluateLayer(EOutbreakBehaviorLayer Layer, const FOutbreakBehaviorContext& Context,
		EOutbreakTeammateBehavior& OutBehavior);

	/** Walks PriorityOrder and returns the first applicable layer's behaviour (Idle if none applies). */
	OUTBREAK_API FOutbreakBehaviorDecision SelectBehavior(const FOutbreakBehaviorContext& Context,
		const TArray<EOutbreakBehaviorLayer>& PriorityOrder);

	OUTBREAK_API FText GetBehaviorDisplayName(EOutbreakTeammateBehavior Behavior);
}
