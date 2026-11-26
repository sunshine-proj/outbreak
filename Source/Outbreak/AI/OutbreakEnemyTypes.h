#pragma once

#include "CoreMinimal.h"

#include "OutbreakEnemyTypes.generated.h"

/**
 * Enemy awareness states (Phase 8; Phase 5 had Idle / Patrol / Alerted / Combat).
 * "Alerted" in the roadmap is Combat: the enemy has detected a hostile and fights or hunts it.
 */
UENUM(BlueprintType)
enum class EOutbreakEnemyState : uint8
{
	/** Stands at its home position. */
	Idle,
	/** Walks its patrol route. */
	Patrol,
	/** Something is off (a glimpse, a faint noise): stops and looks towards it. */
	Suspicious,
	/** Goes to the stimulus (noise, sighting, being shot, an alert from another enemy). */
	Investigating,
	/** Has detected a hostile: fights it, or closes on where it was last seen. */
	Combat,
	/** Lost a detected hostile: searches around its last known position. */
	Searching
};

/** Inputs to the enemy state decision. Gathered by AOutbreakEnemyAIController. */
struct FOutbreakEnemyStateContext
{
	/** A hostile has been detected (detection reached 1) and is still being tracked. */
	bool bHasDetectedTarget = false;

	float TimeSinceTargetSeen = 0.0f;

	/** Seconds out of sight before a detected target counts as lost. */
	float LoseTargetTime = 3.0f;

	/** A search is in progress (started after losing a target and not yet timed out). */
	bool bSearching = false;

	float Suspicion = 0.0f;
	float SuspiciousThreshold = 0.2f;
	float InvestigateThreshold = 0.5f;

	/** A location to investigate is known. */
	bool bHasStimulusLocation = false;

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
	/**
	 * Combat while a detected target was seen within LoseTargetTime; then Searching while the search
	 * lasts; then Investigating or Suspicious by suspicion; otherwise Patrol (with a route) or Idle.
	 */
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
