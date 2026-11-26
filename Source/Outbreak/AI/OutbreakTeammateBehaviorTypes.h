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
	/** Low health under threat: fall back to cover, or to the player. */
	SeekSafety,
	/** Defend order: stay around the defended position. */
	DefendPosition,
	/** In cover from the threat (Cover order, or autonomously when under fire). */
	TakeCover,
	/** Retreat order: moving away from the threat. */
	Retreat,
	/** Squad stealth: an enemy is noticing this teammate, so it freezes in place. */
	Hiding,
	/** Going to an incapacitated squadmate and reviving them (Phase 10). */
	ReviveMember,
	/** Going to a hurt squadmate and using a medical item on them (Phase 10). */
	HealMember
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
	/** A visible hostile: take cover if under fire and cover is known, otherwise engage. */
	ThreatResponse,
	/** Too far from the follow position. */
	FormationMaintenance,
	/** Nothing else applies. */
	DefaultAutonomous,
	/**
	 * A squadmate needs help (Phase 10): revive a downed one (always), or heal a hurt one with a
	 * medical item (only without an active order). Appended so saved orders keep their values; its
	 * place in the default order is second.
	 */
	MedicalAssistance
};

/** Inputs to the behaviour decision. Gathered by the AI controller. */
struct FOutbreakBehaviorContext
{
	/** The member's order, or None if it has no active order. */
	EOutbreakSquadCommandType ActiveCommand = EOutbreakSquadCommandType::None;

	bool bHasThreat = false;
	bool bLowHealth = false;

	/** Took damage recently. */
	bool bUnderFire = false;

	/** A cover position from the current threat is known. */
	bool bHasCover = false;
	bool bHasPlayer = false;

	/** Distance from the member to its follow position near the player. */
	float DistanceToFollowLocation = 0.0f;

	/** This teammate is the one assigned to revive a downed squadmate. */
	bool bHasDownedSquadmate = false;

	/** This teammate is the one assigned to heal a hurt squadmate, and carries a medical item. */
	bool bHasHurtSquadmate = false;

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
	/**
	 * Emergency survival > medical assistance > explicit order > threat response > formation >
	 * default (AGENTS.md §6.1; medical assistance added in Phase 10, ahead of orders because a downed
	 * squadmate bleeds out; healing still yields to orders).
	 */
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

	/**
	 * Squad stealth: when an enemy is noticing the teammate (bShouldHide), movement behaviours
	 * (following, moving, regrouping, retreating, idling) become Hiding so it doesn't give the squad
	 * away. Survival, holding, defending, cover and attack orders are kept.
	 */
	OUTBREAK_API EOutbreakTeammateBehavior ApplyStealthOverride(EOutbreakTeammateBehavior Behavior, bool bShouldHide);
}
