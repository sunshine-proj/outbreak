#pragma once

#include "CoreMinimal.h"

#include "OutbreakEnemyTactics.generated.h"

class UEnvQuery;

/** How an archetype closes with its target (Phase 9). */
UENUM(BlueprintType)
enum class EOutbreakEnemyCombatStyle : uint8
{
	/** Keeps to the combat range band, uses cover and flanks (alien infantry). */
	Ranged,
	/** Runs straight at the target and attacks at reach (fast melee attacker). */
	Melee,
	/** Walks steadily towards PreferredCombatRange, ignoring cover (heavy). */
	Advance
};

/** What an enemy in combat is doing with its movement. Firing is concurrent with all of them. */
UENUM(BlueprintType)
enum class EOutbreakEnemyCombatMove : uint8
{
	/** Not in combat. */
	None,
	/** Range band (Ranged) or steady advance (Advance). */
	Engage,
	/** Moving to, or holding, a cover position from the cover EQS query. */
	TakeCover,
	/** Moving round the target's side while an ally pins it from the front. */
	Flank,
	/** Running at the target (Melee). */
	Charge,
	/** Falling back away from the threat. */
	Retreat
};

/**
 * Combat tactics of an enemy archetype (UOutbreakEnemyDefinition::Tactics). Everything that makes
 * archetypes behave differently in a fight is data here, not code (Phase 9 acceptance).
 */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakEnemyTacticsSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
	EOutbreakEnemyCombatStyle CombatStyle = EOutbreakEnemyCombatStyle::Ranged;

	// --- Cover ---

	/**
	 * EQS query returning cover from the "Outbreak: Threat" context near the "Outbreak: Cover Anchor"
	 * context (the same contexts as the teammates; Docs/Phase9-EditorSetup.md). Empty = no cover.
	 * Ranged style only.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UEnvQuery> CoverQuery;

	/** Fight from cover whenever there is some; otherwise only when under fire or reloading. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover")
	bool bPreferCover = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover", meta = (ClampMin = "0.1", Units = "s"))
	float CoverQueryInterval = 2.0f;

	/** Counts as under fire for this long after being damaged. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover", meta = (ClampMin = "0.0", Units = "s"))
	float UnderFireTime = 2.5f;

	/** In cover without sight of the target for this long: the cover is dropped and a new one sought. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover", meta = (ClampMin = "0.0", Units = "s"))
	float CoverRepositionTime = 4.0f;

	// --- Flanking and coordination ---

	/** May take a flanking role when allies engage the same target. Ranged and Advance styles. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flanking")
	bool bCanFlank = true;

	/** Angle round the target, from the pinning ally's line of attack, of the flank position. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flanking", meta = (ClampMin = "20.0", ClampMax = "170.0", Units = "deg"))
	float FlankAngle = 75.0f;

	/** At most this many enemies flank one target (counted from this enemy's definition). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flanking", meta = (ClampMin = "0"))
	int32 MaxFlankersPerTarget = 2;

	/**
	 * Allies within this distance share sightings of their targets and coordinate roles (pin, flank)
	 * and target choice. 0 = fights alone.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flanking", meta = (ClampMin = "0.0", Units = "cm"))
	float CoordinationRadius = 2500.0f;

	// --- Target selection ---

	/** A new target must score this much more than the current visible one before switching. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting", meta = (ClampMin = "0.0"))
	float TargetSwitchMargin = 5.0f;

	/** Score lost per coordinating ally already fighting a target, spreading fire across the squad. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting", meta = (ClampMin = "0.0"))
	float SharedTargetPenalty = 4.0f;

	// --- Retreat and disengagement ---

	/** Retreats once when health falls to this fraction. 0 = never for health. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retreat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RetreatHealthFraction = 0.25f;

	/**
	 * Retreats when visible hostiles are at least this many times the coordinating allies (itself
	 * included). 0 = never for numbers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retreat", meta = (ClampMin = "0.0"))
	float OutnumberedRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retreat", meta = (ClampMin = "0.0", Units = "cm"))
	float RetreatDistance = 1500.0f;

	/** Seconds a retreat lasts before the enemy fights normally again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retreat", meta = (ClampMin = "0.0", Units = "s"))
	float RetreatDuration = 6.0f;

	/** Seconds before a retreat for numbers can happen again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retreat", meta = (ClampMin = "0.0", Units = "s"))
	float RetreatCooldown = 20.0f;

	/**
	 * How far from its home position the enemy will fight. Combat positions are kept inside it, and a
	 * target lost beyond it is not searched for: the enemy disengages and returns. 0 = unlimited.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retreat", meta = (ClampMin = "0.0", Units = "cm"))
	float LeashDistance = 0.0f;

	/** Appends a reason for each invalid setting. */
	void Validate(TArray<FText>& OutErrors) const;
};

/** Inputs to the combat movement decision. Gathered by AOutbreakEnemyAIController. */
struct FOutbreakEnemyCombatContext
{
	EOutbreakEnemyCombatStyle Style = EOutbreakEnemyCombatStyle::Ranged;
	bool bRetreating = false;
	bool bTargetVisible = false;

	/** A cover position is known (Ranged style with a cover query). */
	bool bHasCover = false;
	bool bPreferCover = false;
	bool bUnderFire = false;
	bool bReloading = false;

	/** The coordination assigned this enemy a flanking role and the flank position is reachable. */
	bool bFlankRole = false;
	/** Already at an angle round the target where the flank counts as made. */
	bool bFlankReached = false;
};

/** One enemy fighting a target, for role assignment. */
struct FOutbreakEngagementCandidate
{
	float DistanceToTarget = 0.0f;
	bool bCanFlank = false;
	/** Was flanking at the previous assignment (keeps roles stable). */
	bool bWasFlanking = false;
};

/** One possible target, for target selection. */
struct FOutbreakTargetCandidate
{
	float DistanceCm = 0.0f;
	bool bIsRecentAttacker = false;
	bool bIsPlayer = false;
	/** Coordinating allies already fighting this target. */
	int32 OtherAttackers = 0;
};

struct FOutbreakTargetScoreWeights;

namespace OutbreakEnemyTactics
{
	/** Retreat > Charge (melee) > Flank (until made) > TakeCover (preferred, under fire or reloading) > Engage. */
	OUTBREAK_API EOutbreakEnemyCombatMove SelectCombatMove(const FOutbreakEnemyCombatContext& Context);

	/**
	 * Where to stand when engaging. Melee: at the target. Advance: close to PreferredRange and hold
	 * (never backs off). Ranged: the Phase 5 range band (ComputeCombatPosition).
	 */
	OUTBREAK_API FVector ComputeEngagePosition(EOutbreakEnemyCombatStyle Style, const FVector& SelfLocation,
		const FVector& TargetLocation, bool bTargetVisible, float MinRange, float PreferredRange, float MaxRange);

	/**
	 * The flank position: the pinning ally's line of attack (target -> PinLocation) rotated by
	 * FlankAngle round the target to Side (+1 = counter-clockwise seen from above, -1 = clockwise),
	 * at Distance from the target.
	 */
	OUTBREAK_API FVector ComputeFlankPosition(const FVector& TargetLocation, const FVector& PinLocation,
		float FlankAngleDegrees, int32 Side, float Distance);

	/** The flank is made once Self is at least 75% of FlankAngle round the target from the pin. */
	OUTBREAK_API bool HasFlanked(const FVector& TargetLocation, const FVector& PinLocation, const FVector& SelfLocation,
		float FlankAngleDegrees);

	/**
	 * Squad-aware side choice: the side (+1/-1) whose flank position is farther from the target's
	 * other squad members, so the flanker does not walk into them. Ties go to +1.
	 */
	OUTBREAK_API int32 ChooseFlankSide(const FVector& LeftPosition, const FVector& RightPosition,
		TConstArrayView<FVector> OtherHostileLocations);

	/** Distance Distance straight away from the threat (2D). Self when there is no direction. */
	OUTBREAK_API FVector ComputeRetreatPosition(const FVector& SelfLocation, const FVector& ThreatLocation, float Distance);

	/** Health at or below RetreatHealthFraction (and above 0), unless a health retreat already happened. */
	OUTBREAK_API bool ShouldRetreatForHealth(float HealthFraction, float RetreatHealthFraction, bool bAlreadyRetreatedForHealth);

	/** VisibleHostiles >= Allies (itself included) x OutnumberedRatio. Never with a ratio of 0 or no hostiles. */
	OUTBREAK_API bool IsOutnumbered(int32 VisibleHostiles, int32 Allies, float OutnumberedRatio);

	/** Position pulled back inside LeashDistance of Home (2D). LeashDistance 0 = unlimited. */
	OUTBREAK_API FVector ClampToLeash(const FVector& Position, const FVector& Home, float LeashDistance);

	/** A lost target is searched for only when its last known position is inside the leash. */
	OUTBREAK_API bool IsWithinLeash(const FVector& Position, const FVector& Home, float LeashDistance);
}

namespace OutbreakEnemyCoordination
{
	/**
	 * Roles of the enemies fighting one target. Returns one entry per candidate: INDEX_NONE = pin
	 * (engage from the front), 0, 1, ... = flank order (even ordinals take the preferred side, odd
	 * ones the other). The pin is the nearest candidate that cannot flank, else the nearest; then up
	 * to MaxFlankers flank-capable candidates flank, current flankers first, then by distance.
	 * A lone enemy never flanks.
	 */
	OUTBREAK_API TArray<int32> AssignFlankRoles(TConstArrayView<FOutbreakEngagementCandidate> Candidates, int32 MaxFlankers);

	/** The Phase 5 target score minus SharedTargetPenalty per ally already fighting the target. */
	OUTBREAK_API float ScoreCandidate(const FOutbreakTargetCandidate& Candidate, const FOutbreakTargetScoreWeights& Weights,
		float SharedTargetPenalty);

	/**
	 * Index of the target to fight. Keeps CurrentIndex (if valid) unless another scores more than
	 * SwitchMargin higher. INDEX_NONE when Scores is empty.
	 */
	OUTBREAK_API int32 SelectTarget(TConstArrayView<float> Scores, int32 CurrentIndex, float SwitchMargin);
}
