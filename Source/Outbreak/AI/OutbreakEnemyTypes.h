#pragma once

#include "CoreMinimal.h"

#include "OutbreakEnemyTypes.generated.h"

/**
 * Basic enemy states (Phase 5). Phase 8 refines Alerted into Suspicious, Investigating and
 * Searching with suspicion levels.
 */
UENUM(BlueprintType)
enum class EOutbreakEnemyState : uint8
{
	/** Stands at its home position. */
	Idle,
	/** Walks its patrol route. */
	Patrol,
	/** Heard a shot, was hit by an unseen attacker, or lost its target: goes to look. */
	Alerted,
	/** Sees a hostile: positions and fights. */
	Combat
};

/** Inputs to the enemy state decision. Gathered by AOutbreakEnemyAIController. */
struct FOutbreakEnemyStateContext
{
	bool bHasVisibleTarget = false;

	/** An alert location exists (noise, damage or last-seen target). */
	bool bHasAlert = false;

	/** Seconds since the latest alert stimulus. */
	float AlertElapsed = 0.0f;

	/** How long an alert lasts without new stimuli. */
	float AlertDuration = 12.0f;

	bool bHasPatrolRoute = false;
};

/** Target scoring weights. Higher score = more attractive target. */
struct FOutbreakTargetScoreWeights
{
	/** Score lost per metre of distance. */
	float DistancePenaltyPerMeter = 1.0f;

	/** Bonus for whoever damaged this enemy recently. */
	float RecentAttackerBonus = 20.0f;

	/** Bonus for the human player (vs AI teammates). */
	float PlayerBonus = 5.0f;
};

namespace OutbreakEnemyBehavior
{
	/** Combat if a hostile is visible; Alerted while an alert is fresh; otherwise Patrol (with a route) or Idle. */
	OUTBREAK_API EOutbreakEnemyState SelectState(const FOutbreakEnemyStateContext& Context);

	OUTBREAK_API FText GetStateDisplayName(EOutbreakEnemyState State);
}

namespace OutbreakEnemyTargeting
{
	OUTBREAK_API float ScoreTarget(float DistanceCm, bool bIsRecentAttacker, bool bIsPlayer, const FOutbreakTargetScoreWeights& Weights);
}

namespace OutbreakEnemyTactics
{
	/**
	 * Where to stand to fight Target. Without line of sight: close in to MinRange to regain it.
	 * Visible beyond MaxRange or within MinRange: move to PreferredRange along the line to the target.
	 * Visible inside the band: hold (returns SelfLocation).
	 */
	OUTBREAK_API FVector ComputeCombatPosition(const FVector& SelfLocation, const FVector& TargetLocation, bool bTargetVisible,
		float MinRange, float PreferredRange, float MaxRange);
}
