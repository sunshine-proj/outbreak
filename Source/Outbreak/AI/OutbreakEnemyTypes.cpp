#include "AI/OutbreakEnemyTypes.h"

#define LOCTEXT_NAMESPACE "OutbreakEnemy"

namespace OutbreakEnemyBehavior
{
	EOutbreakEnemyState SelectState(const FOutbreakEnemyStateContext& Context)
	{
		if (Context.bRetreating)
		{
			return EOutbreakEnemyState::Retreating;
		}
		if (Context.bHasDetectedTarget && Context.TimeSinceTargetSeen <= Context.LoseTargetTime)
		{
			return EOutbreakEnemyState::Combat;
		}
		if (Context.bSearching)
		{
			return EOutbreakEnemyState::Searching;
		}
		if (Context.bHasStimulusLocation && Context.Suspicion >= Context.InvestigateThreshold)
		{
			return EOutbreakEnemyState::Investigating;
		}
		if (Context.Suspicion >= Context.SuspiciousThreshold)
		{
			return EOutbreakEnemyState::Suspicious;
		}
		return Context.bHasPatrolRoute ? EOutbreakEnemyState::Patrol : EOutbreakEnemyState::Idle;
	}

	FText GetStateDisplayName(EOutbreakEnemyState State)
	{
		switch (State)
		{
		case EOutbreakEnemyState::Patrol:        return LOCTEXT("Patrol", "Patrol");
		case EOutbreakEnemyState::Suspicious:    return LOCTEXT("Suspicious", "Suspicious");
		case EOutbreakEnemyState::Investigating: return LOCTEXT("Investigating", "Investigating");
		case EOutbreakEnemyState::Combat:        return LOCTEXT("Combat", "Combat");
		case EOutbreakEnemyState::Searching:     return LOCTEXT("Searching", "Searching");
		case EOutbreakEnemyState::Retreating:    return LOCTEXT("Retreating", "Retreating");
		default:                                 return LOCTEXT("Idle", "Idle");
		}
	}
}

namespace OutbreakEnemyTargeting
{
	float ScoreTarget(float DistanceCm, bool bIsRecentAttacker, bool bIsPlayer, const FOutbreakTargetScoreWeights& Weights)
	{
		float Score = -FMath::Max(0.0f, DistanceCm) / 100.0f * Weights.DistancePenaltyPerMeter;
		if (bIsRecentAttacker)
		{
			Score += Weights.RecentAttackerBonus;
		}
		if (bIsPlayer)
		{
			Score += Weights.PlayerBonus;
		}
		return Score;
	}
}

namespace OutbreakEnemyTactics
{
	FVector ComputeCombatPosition(const FVector& SelfLocation, const FVector& TargetLocation, bool bTargetVisible,
		float MinRange, float PreferredRange, float MaxRange)
	{
		const FVector FromTarget = (SelfLocation - TargetLocation).GetSafeNormal2D();
		const float Distance = FVector::Dist2D(SelfLocation, TargetLocation);

		if (!bTargetVisible)
		{
			return Distance > MinRange ? TargetLocation + FromTarget * MinRange : SelfLocation;
		}
		if (Distance > MaxRange || Distance < MinRange)
		{
			// Directly on top of the target there is no direction to back away in; hold instead.
			return FromTarget.IsNearlyZero() ? SelfLocation : TargetLocation + FromTarget * PreferredRange;
		}
		return SelfLocation;
	}
}

#undef LOCTEXT_NAMESPACE
