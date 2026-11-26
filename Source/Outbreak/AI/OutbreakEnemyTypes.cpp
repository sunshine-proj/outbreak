#include "AI/OutbreakEnemyTypes.h"

#define LOCTEXT_NAMESPACE "OutbreakEnemy"

namespace OutbreakEnemyBehavior
{
	EOutbreakEnemyState SelectState(const FOutbreakEnemyStateContext& Context)
	{
		if (Context.bHasVisibleTarget)
		{
			return EOutbreakEnemyState::Combat;
		}
		if (Context.bHasAlert && Context.AlertElapsed < Context.AlertDuration)
		{
			return EOutbreakEnemyState::Alerted;
		}
		return Context.bHasPatrolRoute ? EOutbreakEnemyState::Patrol : EOutbreakEnemyState::Idle;
	}

	FText GetStateDisplayName(EOutbreakEnemyState State)
	{
		switch (State)
		{
		case EOutbreakEnemyState::Patrol:  return LOCTEXT("Patrol", "Patrol");
		case EOutbreakEnemyState::Alerted: return LOCTEXT("Alerted", "Alerted");
		case EOutbreakEnemyState::Combat:  return LOCTEXT("Combat", "Combat");
		default:                           return LOCTEXT("Idle", "Idle");
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
