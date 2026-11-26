#include "AI/OutbreakTeammateBehaviorTypes.h"

#define LOCTEXT_NAMESPACE "OutbreakTeammateBehavior"

namespace OutbreakTeammateBehavior
{
	TArray<EOutbreakBehaviorLayer> GetDefaultPriorityOrder()
	{
		return {
			EOutbreakBehaviorLayer::EmergencySurvival,
			EOutbreakBehaviorLayer::ExplicitOrder,
			EOutbreakBehaviorLayer::ThreatResponse,
			EOutbreakBehaviorLayer::FormationMaintenance,
			EOutbreakBehaviorLayer::DefaultAutonomous
		};
	}

	bool IsValidPriorityOrder(const TArray<EOutbreakBehaviorLayer>& PriorityOrder)
	{
		const TArray<EOutbreakBehaviorLayer> AllLayers = GetDefaultPriorityOrder();
		if (PriorityOrder.Num() != AllLayers.Num())
		{
			return false;
		}
		for (const EOutbreakBehaviorLayer Layer : AllLayers)
		{
			if (!PriorityOrder.Contains(Layer))
			{
				return false;
			}
		}
		return true;
	}

	bool EvaluateLayer(EOutbreakBehaviorLayer Layer, const FOutbreakBehaviorContext& Context, EOutbreakTeammateBehavior& OutBehavior)
	{
		switch (Layer)
		{
		case EOutbreakBehaviorLayer::EmergencySurvival:
			if (Context.bLowHealth && Context.bHasThreat && Context.bHasPlayer)
			{
				OutBehavior = EOutbreakTeammateBehavior::SeekSafety;
				return true;
			}
			return false;

		case EOutbreakBehaviorLayer::ExplicitOrder:
			switch (Context.ActiveCommand)
			{
			case EOutbreakSquadCommandType::Follow:
				OutBehavior = EOutbreakTeammateBehavior::FollowPlayer;
				return Context.bHasPlayer;
			case EOutbreakSquadCommandType::Hold:
				OutBehavior = EOutbreakTeammateBehavior::HoldPosition;
				return true;
			case EOutbreakSquadCommandType::Move:
				OutBehavior = EOutbreakTeammateBehavior::MoveToLocation;
				return true;
			case EOutbreakSquadCommandType::Regroup:
				OutBehavior = EOutbreakTeammateBehavior::Regroup;
				return Context.bHasPlayer;
			case EOutbreakSquadCommandType::Attack:
				OutBehavior = EOutbreakTeammateBehavior::AttackTarget;
				return true;
			default:
				return false;
			}

		case EOutbreakBehaviorLayer::ThreatResponse:
			if (Context.bHasThreat)
			{
				OutBehavior = EOutbreakTeammateBehavior::EngageThreat;
				return true;
			}
			return false;

		case EOutbreakBehaviorLayer::FormationMaintenance:
		{
			const float Threshold = Context.bCurrentlyFollowing ? Context.FollowStopDistance : Context.FollowStartDistance;
			if (Context.bHasPlayer && Context.DistanceToFollowLocation > Threshold)
			{
				OutBehavior = EOutbreakTeammateBehavior::FollowPlayer;
				return true;
			}
			return false;
		}

		case EOutbreakBehaviorLayer::DefaultAutonomous:
			OutBehavior = EOutbreakTeammateBehavior::Idle;
			return true;

		default:
			return false;
		}
	}

	FOutbreakBehaviorDecision SelectBehavior(const FOutbreakBehaviorContext& Context, const TArray<EOutbreakBehaviorLayer>& PriorityOrder)
	{
		FOutbreakBehaviorDecision Decision;
		for (const EOutbreakBehaviorLayer Layer : PriorityOrder)
		{
			EOutbreakTeammateBehavior Behavior = EOutbreakTeammateBehavior::Idle;
			if (EvaluateLayer(Layer, Context, Behavior))
			{
				Decision.Behavior = Behavior;
				Decision.Layer = Layer;
				return Decision;
			}
		}
		return Decision;
	}

	FText GetBehaviorDisplayName(EOutbreakTeammateBehavior Behavior)
	{
		switch (Behavior)
		{
		case EOutbreakTeammateBehavior::FollowPlayer:   return LOCTEXT("Following", "Following");
		case EOutbreakTeammateBehavior::HoldPosition:   return LOCTEXT("Holding", "Holding");
		case EOutbreakTeammateBehavior::MoveToLocation: return LOCTEXT("Moving", "Moving");
		case EOutbreakTeammateBehavior::Regroup:        return LOCTEXT("Regrouping", "Regrouping");
		case EOutbreakTeammateBehavior::AttackTarget:   return LOCTEXT("Attacking", "Attacking");
		case EOutbreakTeammateBehavior::EngageThreat:   return LOCTEXT("Engaging", "Engaging");
		case EOutbreakTeammateBehavior::SeekSafety:     return LOCTEXT("FallingBack", "Falling back");
		default:                                        return LOCTEXT("Idle", "Idle");
		}
	}
}

#undef LOCTEXT_NAMESPACE
