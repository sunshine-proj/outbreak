#include "AI/OutbreakEnemyTactics.h"

#include "AI/OutbreakEnemyTypes.h"

#define LOCTEXT_NAMESPACE "OutbreakEnemyTactics"

void FOutbreakEnemyTacticsSettings::Validate(TArray<FText>& OutErrors) const
{
	if (FlankAngle < 20.0f || FlankAngle > 170.0f)
	{
		OutErrors.Add(LOCTEXT("FlankAngle", "Tactics: FlankAngle must be between 20 and 170 degrees."));
	}
	if (RetreatHealthFraction < 0.0f || RetreatHealthFraction > 1.0f)
	{
		OutErrors.Add(LOCTEXT("RetreatHealth", "Tactics: RetreatHealthFraction must be between 0 and 1."));
	}
	if (OutnumberedRatio < 0.0f || RetreatDistance < 0.0f || RetreatDuration < 0.0f || LeashDistance < 0.0f
		|| CoordinationRadius < 0.0f || MaxFlankersPerTarget < 0)
	{
		OutErrors.Add(LOCTEXT("Negative", "Tactics: distances, durations, ratios and counts must not be negative."));
	}
}

namespace OutbreakEnemyTactics
{
	EOutbreakEnemyCombatMove SelectCombatMove(const FOutbreakEnemyCombatContext& Context)
	{
		if (Context.bRetreating)
		{
			return EOutbreakEnemyCombatMove::Retreat;
		}
		if (Context.Style == EOutbreakEnemyCombatStyle::Melee)
		{
			return EOutbreakEnemyCombatMove::Charge;
		}
		if (Context.bFlankRole && !Context.bFlankReached)
		{
			return EOutbreakEnemyCombatMove::Flank;
		}
		if (Context.Style == EOutbreakEnemyCombatStyle::Ranged && Context.bHasCover
			&& (Context.bPreferCover || Context.bUnderFire || Context.bReloading))
		{
			return EOutbreakEnemyCombatMove::TakeCover;
		}
		return EOutbreakEnemyCombatMove::Engage;
	}

	FVector ComputeEngagePosition(EOutbreakEnemyCombatStyle Style, const FVector& SelfLocation,
		const FVector& TargetLocation, bool bTargetVisible, float MinRange, float PreferredRange, float MaxRange)
	{
		switch (Style)
		{
		case EOutbreakEnemyCombatStyle::Melee:
			return TargetLocation;

		case EOutbreakEnemyCombatStyle::Advance:
		{
			const float Distance = FVector::Dist2D(SelfLocation, TargetLocation);
			if (!bTargetVisible || Distance > PreferredRange)
			{
				// Close in to the preferred range (or, without sight, to the minimum range).
				const float StopDistance = bTargetVisible ? PreferredRange : MinRange;
				const FVector FromTarget = (SelfLocation - TargetLocation).GetSafeNormal2D();
				return Distance > StopDistance ? TargetLocation + FromTarget * StopDistance : SelfLocation;
			}
			return SelfLocation;
		}

		case EOutbreakEnemyCombatStyle::Ranged:
		default:
			return ComputeCombatPosition(SelfLocation, TargetLocation, bTargetVisible, MinRange, PreferredRange, MaxRange);
		}
	}

	FVector ComputeFlankPosition(const FVector& TargetLocation, const FVector& PinLocation,
		float FlankAngleDegrees, int32 Side, float Distance)
	{
		FVector Front = (PinLocation - TargetLocation).GetSafeNormal2D();
		if (Front.IsNearlyZero())
		{
			Front = FVector::ForwardVector;
		}
		const float Angle = FlankAngleDegrees * (Side >= 0 ? 1.0f : -1.0f);
		const FVector Direction = Front.RotateAngleAxis(Angle, FVector::UpVector);
		return TargetLocation + Direction * Distance;
	}

	bool HasFlanked(const FVector& TargetLocation, const FVector& PinLocation, const FVector& SelfLocation,
		float FlankAngleDegrees)
	{
		const FVector Front = (PinLocation - TargetLocation).GetSafeNormal2D();
		const FVector ToSelf = (SelfLocation - TargetLocation).GetSafeNormal2D();
		if (Front.IsNearlyZero() || ToSelf.IsNearlyZero())
		{
			return false;
		}
		const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Front, ToSelf), -1.0f, 1.0f)));
		return AngleDegrees >= FlankAngleDegrees * 0.75f;
	}

	int32 ChooseFlankSide(const FVector& LeftPosition, const FVector& RightPosition,
		TConstArrayView<FVector> OtherHostileLocations)
	{
		auto NearestDistance = [&OtherHostileLocations](const FVector& Position)
		{
			float Nearest = TNumericLimits<float>::Max();
			for (const FVector& Hostile : OtherHostileLocations)
			{
				Nearest = FMath::Min(Nearest, static_cast<float>(FVector::Dist2D(Position, Hostile)));
			}
			return Nearest;
		};
		// Left is the +1 side.
		return NearestDistance(LeftPosition) >= NearestDistance(RightPosition) ? 1 : -1;
	}

	FVector ComputeRetreatPosition(const FVector& SelfLocation, const FVector& ThreatLocation, float Distance)
	{
		const FVector Away = (SelfLocation - ThreatLocation).GetSafeNormal2D();
		return Away.IsNearlyZero() ? SelfLocation : SelfLocation + Away * Distance;
	}

	bool ShouldRetreatForHealth(float HealthFraction, float RetreatHealthFraction, bool bAlreadyRetreatedForHealth)
	{
		return !bAlreadyRetreatedForHealth && RetreatHealthFraction > 0.0f
			&& HealthFraction > 0.0f && HealthFraction <= RetreatHealthFraction;
	}

	bool IsOutnumbered(int32 VisibleHostiles, int32 Allies, float OutnumberedRatio)
	{
		return OutnumberedRatio > 0.0f && VisibleHostiles > 0
			&& VisibleHostiles >= FMath::Max(1, Allies) * OutnumberedRatio;
	}

	FVector ClampToLeash(const FVector& Position, const FVector& Home, float LeashDistance)
	{
		if (LeashDistance <= 0.0f)
		{
			return Position;
		}
		const FVector Offset = Position - Home;
		const float Distance2D = Offset.Size2D();
		if (Distance2D <= LeashDistance)
		{
			return Position;
		}
		const FVector Clamped2D = FVector(Offset.X, Offset.Y, 0.0f) * (LeashDistance / Distance2D);
		return FVector(Home.X + Clamped2D.X, Home.Y + Clamped2D.Y, Position.Z);
	}

	bool IsWithinLeash(const FVector& Position, const FVector& Home, float LeashDistance)
	{
		return LeashDistance <= 0.0f || FVector::Dist2D(Position, Home) <= LeashDistance;
	}
}

namespace OutbreakEnemyCoordination
{
	TArray<int32> AssignFlankRoles(TConstArrayView<FOutbreakEngagementCandidate> Candidates, int32 MaxFlankers)
	{
		TArray<int32> Roles;
		Roles.Init(INDEX_NONE, Candidates.Num());
		if (Candidates.Num() < 2 || MaxFlankers <= 0)
		{
			return Roles;
		}

		// The pin: nearest non-flanker, else nearest.
		int32 Pin = INDEX_NONE;
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			const bool bBetterKind = Pin == INDEX_NONE || (!Candidates[Index].bCanFlank && Candidates[Pin].bCanFlank);
			const bool bSameKindCloser = Pin != INDEX_NONE && Candidates[Index].bCanFlank == Candidates[Pin].bCanFlank
				&& Candidates[Index].DistanceToTarget < Candidates[Pin].DistanceToTarget;
			if (bBetterKind || bSameKindCloser)
			{
				Pin = Index;
			}
		}

		TArray<int32> Flankers;
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			if (Index != Pin && Candidates[Index].bCanFlank)
			{
				Flankers.Add(Index);
			}
		}
		Flankers.StableSort([&Candidates](int32 A, int32 B)
		{
			if (Candidates[A].bWasFlanking != Candidates[B].bWasFlanking)
			{
				return Candidates[A].bWasFlanking;
			}
			return Candidates[A].DistanceToTarget < Candidates[B].DistanceToTarget;
		});

		for (int32 Order = 0; Order < FMath::Min(MaxFlankers, Flankers.Num()); ++Order)
		{
			Roles[Flankers[Order]] = Order;
		}
		return Roles;
	}

	float ScoreCandidate(const FOutbreakTargetCandidate& Candidate, const FOutbreakTargetScoreWeights& Weights,
		float SharedTargetPenalty)
	{
		return OutbreakEnemyTargeting::ScoreTarget(Candidate.DistanceCm, Candidate.bIsRecentAttacker, Candidate.bIsPlayer, Weights)
			- FMath::Max(0, Candidate.OtherAttackers) * SharedTargetPenalty;
	}

	int32 SelectTarget(TConstArrayView<float> Scores, int32 CurrentIndex, float SwitchMargin)
	{
		int32 Best = INDEX_NONE;
		for (int32 Index = 0; Index < Scores.Num(); ++Index)
		{
			if (Best == INDEX_NONE || Scores[Index] > Scores[Best])
			{
				Best = Index;
			}
		}
		if (Scores.IsValidIndex(CurrentIndex) && Best != CurrentIndex && Scores[Best] <= Scores[CurrentIndex] + SwitchMargin)
		{
			return CurrentIndex;
		}
		return Best;
	}
}

#undef LOCTEXT_NAMESPACE
