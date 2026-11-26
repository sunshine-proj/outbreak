#include "AI/OutbreakEnemyCoordinationSubsystem.h"

#include "AI/OutbreakEnemyAIController.h"
#include "AI/OutbreakEnemyTactics.h"
#include "Engine/World.h"

double UOutbreakEnemyCoordinationSubsystem::GetTimeSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

void UOutbreakEnemyCoordinationSubsystem::PruneExpired()
{
	const double Now = GetTimeSeconds();
	Engagements.RemoveAll([Now](const FEngagement& Entry)
	{
		return !Entry.Enemy.IsValid() || !Entry.Target.IsValid() || Now - Entry.UpdateTime > EngagementLifetime;
	});
}

const UOutbreakEnemyCoordinationSubsystem::FEngagement* UOutbreakEnemyCoordinationSubsystem::FindEngagement(
	const AOutbreakEnemyAIController* Enemy) const
{
	return Engagements.FindByPredicate([Enemy](const FEngagement& Entry) { return Entry.Enemy.Get() == Enemy; });
}

void UOutbreakEnemyCoordinationSubsystem::ReportEngagement(AOutbreakEnemyAIController* Enemy, AActor* Target,
	const FVector& EnemyLocation, bool bCanFlank, bool bTargetSeen, const FVector& SeenTargetLocation)
{
	if (!Enemy)
	{
		return;
	}
	if (!Target)
	{
		RemoveEngagement(Enemy);
		return;
	}

	PruneExpired();

	FEngagement* Entry = Engagements.FindByPredicate([Enemy](const FEngagement& Existing) { return Existing.Enemy.Get() == Enemy; });
	if (!Entry)
	{
		Entry = &Engagements.AddDefaulted_GetRef();
		Entry->Enemy = Enemy;
	}
	if (Entry->Target.Get() != Target)
	{
		Entry->Target = Target;
		Entry->bWasFlanking = false;
		Entry->SightingTime = -1.0e9;
	}
	Entry->EnemyLocation = EnemyLocation;
	Entry->bCanFlank = bCanFlank;
	Entry->UpdateTime = GetTimeSeconds();
	if (bTargetSeen)
	{
		Entry->SightingLocation = SeenTargetLocation;
		Entry->SightingTime = Entry->UpdateTime;
	}
}

void UOutbreakEnemyCoordinationSubsystem::RemoveEngagement(const AOutbreakEnemyAIController* Enemy)
{
	Engagements.RemoveAll([Enemy](const FEngagement& Entry) { return Entry.Enemy.Get() == Enemy; });
}

AActor* UOutbreakEnemyCoordinationSubsystem::GetEngagedTarget(const AOutbreakEnemyAIController* Enemy) const
{
	const FEngagement* Entry = FindEngagement(Enemy);
	return Entry ? Entry->Target.Get() : nullptr;
}

int32 UOutbreakEnemyCoordinationSubsystem::CountAlliesEngaging(const AActor* Target, const FVector& Location, float Radius,
	const AOutbreakEnemyAIController* Exclude) const
{
	const double Now = GetTimeSeconds();
	int32 Count = 0;
	for (const FEngagement& Entry : Engagements)
	{
		if (Entry.Enemy.Get() != Exclude && Entry.Enemy.IsValid() && Entry.Target.Get() == Target
			&& Now - Entry.UpdateTime <= EngagementLifetime && FVector::Dist(Entry.EnemyLocation, Location) <= Radius)
		{
			++Count;
		}
	}
	return Count;
}

int32 UOutbreakEnemyCoordinationSubsystem::CountAlliesInCombat(const FVector& Location, float Radius,
	const AOutbreakEnemyAIController* Exclude) const
{
	const double Now = GetTimeSeconds();
	int32 Count = 0;
	for (const FEngagement& Entry : Engagements)
	{
		if (Entry.Enemy.Get() != Exclude && Entry.Enemy.IsValid() && Entry.Target.IsValid()
			&& Now - Entry.UpdateTime <= EngagementLifetime && FVector::Dist(Entry.EnemyLocation, Location) <= Radius)
		{
			++Count;
		}
	}
	return Count;
}

bool UOutbreakEnemyCoordinationSubsystem::GetLatestSighting(const AActor* Target, const FVector& Location, float Radius,
	const AOutbreakEnemyAIController* Exclude, FVector& OutLocation, double& OutTime) const
{
	bool bFound = false;
	for (const FEngagement& Entry : Engagements)
	{
		const bool bHasSighting = Entry.SightingTime > -1.0e8;
		if (Entry.Enemy.Get() == Exclude || !Entry.Enemy.IsValid() || Entry.Target.Get() != Target || !bHasSighting
			|| FVector::Dist(Entry.EnemyLocation, Location) > Radius)
		{
			continue;
		}
		if (!bFound || Entry.SightingTime > OutTime)
		{
			bFound = true;
			OutLocation = Entry.SightingLocation;
			OutTime = Entry.SightingTime;
		}
	}
	return bFound;
}

FOutbreakFlankAssignment UOutbreakEnemyCoordinationSubsystem::GetFlankAssignment(const AOutbreakEnemyAIController* Enemy,
	float Radius, int32 MaxFlankers)
{
	FOutbreakFlankAssignment Assignment;
	const FEngagement* Self = FindEngagement(Enemy);
	const AActor* Target = Self ? Self->Target.Get() : nullptr;
	if (!Self || !Target)
	{
		return Assignment;
	}

	const double Now = GetTimeSeconds();
	TArray<int32> GroupIndices;
	TArray<FOutbreakEngagementCandidate> Candidates;
	for (int32 Index = 0; Index < Engagements.Num(); ++Index)
	{
		const FEngagement& Entry = Engagements[Index];
		if (Entry.Enemy.IsValid() && Entry.Target.Get() == Target && Now - Entry.UpdateTime <= EngagementLifetime
			&& FVector::Dist(Entry.EnemyLocation, Self->EnemyLocation) <= Radius)
		{
			GroupIndices.Add(Index);
			FOutbreakEngagementCandidate& Candidate = Candidates.AddDefaulted_GetRef();
			Candidate.DistanceToTarget = FVector::Dist(Entry.EnemyLocation, Target->GetActorLocation());
			Candidate.bCanFlank = Entry.bCanFlank;
			Candidate.bWasFlanking = Entry.bWasFlanking;
		}
	}

	const TArray<int32> Roles = OutbreakEnemyCoordination::AssignFlankRoles(Candidates, MaxFlankers);
	for (int32 Slot = 0; Slot < GroupIndices.Num(); ++Slot)
	{
		FEngagement& Entry = Engagements[GroupIndices[Slot]];
		Entry.bWasFlanking = Roles[Slot] != INDEX_NONE;
		if (Entry.Enemy.Get() == Enemy)
		{
			Assignment.FlankOrder = Roles[Slot];
		}
	}

	// The pin to flank from: the other group member without a flank role nearest the target.
	float BestPinDistance = TNumericLimits<float>::Max();
	for (int32 Slot = 0; Slot < GroupIndices.Num(); ++Slot)
	{
		if (Roles[Slot] == INDEX_NONE && Candidates[Slot].DistanceToTarget < BestPinDistance
			&& Engagements[GroupIndices[Slot]].Enemy.Get() != Enemy)
		{
			BestPinDistance = Candidates[Slot].DistanceToTarget;
			Assignment.PinLocation = Engagements[GroupIndices[Slot]].EnemyLocation;
		}
	}
	if (Assignment.IsFlanking() && BestPinDistance == TNumericLimits<float>::Max())
	{
		// No pin among the others to flank from: engage instead.
		Assignment.FlankOrder = INDEX_NONE;
	}
	return Assignment;
}
