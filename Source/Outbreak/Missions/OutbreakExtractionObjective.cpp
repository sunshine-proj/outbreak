#include "Missions/OutbreakExtractionObjective.h"

#include "Combat/OutbreakHealthComponent.h"
#include "GameFramework/Pawn.h"
#include "Missions/OutbreakMissionMarker.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Missions/OutbreakObjectiveQueries.h"
#include "Outbreak.h"
#include "Squad/OutbreakSquadMemberComponent.h"

#define LOCTEXT_NAMESPACE "OutbreakObjectives"

void UOutbreakExtractionObjective::Activate(UOutbreakMissionSubsystem& Mission)
{
	Marker = AOutbreakMissionMarker::FindMarker(Mission.GetWorld(), MarkerId);
	LastEvaluationTime = Mission.GetMissionTime();
	HeldTime = 0.0f;
	bWasEveryoneIn = false;
	InZoneCount = 0;
	RequiredCount = 0;
	ClosestPlayerDistance = -1.0f;
	if (!Marker.IsValid())
	{
		UE_LOG(LogOutbreak, Error, TEXT("Extraction objective: no AOutbreakMissionMarker with ID '%s' in the level; the objective fails."),
			*MarkerId.ToString());
	}
}

EOutbreakObjectiveState UOutbreakExtractionObjective::Evaluate(UOutbreakMissionSubsystem& Mission)
{
	const AOutbreakMissionMarker* Zone = Marker.Get();
	if (!Zone)
	{
		return EOutbreakObjectiveState::Failed;
	}

	const float Now = Mission.GetMissionTime();
	const float DeltaTime = FMath::Max(0.0f, Now - LastEvaluationTime);
	LastEvaluationTime = Now;

	const FVector ZoneLocation = Zone->GetActorLocation();
	const TArray<APawn*> LivingPlayers = Mission.GetLivingPlayerPawns();

	// Who has to be in the zone: every standing player, plus standing teammates when required.
	TArray<AActor*> Required;
	for (APawn* Player : LivingPlayers)
	{
		Required.Add(Player);
	}
	if (bRequireTeammates)
	{
		for (AActor* Soldier : Mission.GetSquadSoldiers())
		{
			const UOutbreakSquadMemberComponent* Member = Soldier->FindComponentByClass<UOutbreakSquadMemberComponent>();
			if (Member && !Member->IsPlayerMember() && Member->GetStatus() == EOutbreakSquadMemberStatus::Active)
			{
				Required.AddUnique(Soldier);
			}
		}
	}

	ClosestPlayerDistance = -1.0f;
	for (const APawn* Player : LivingPlayers)
	{
		const float Distance = FVector::Dist(Player->GetActorLocation(), ZoneLocation);
		ClosestPlayerDistance = ClosestPlayerDistance < 0.0f ? Distance : FMath::Min(ClosestPlayerDistance, Distance);
	}

	InZoneCount = 0;
	for (const AActor* Soldier : Required)
	{
		InZoneCount += FVector::Dist(Soldier->GetActorLocation(), ZoneLocation) <= Radius ? 1 : 0;
	}
	RequiredCount = Required.Num();

	const bool bEveryoneIn = !LivingPlayers.IsEmpty() && InZoneCount == RequiredCount;
	if (!bEveryoneIn)
	{
		HeldTime = 0.0f;
		bWasEveryoneIn = false;
		return EOutbreakObjectiveState::Active;
	}

	// The countdown starts when the last soldier arrives, not at the previous evaluation.
	HeldTime += bWasEveryoneIn ? DeltaTime : 0.0f;
	bWasEveryoneIn = true;
	if (HeldTime < ExtractionTime)
	{
		return EOutbreakObjectiveState::Active;
	}

	// Everyone standing in the zone leaves, including teammates that were not required.
	TArray<AActor*> Extracted;
	for (AActor* Soldier : Mission.GetSquadSoldiers())
	{
		const UOutbreakHealthComponent* Health = Soldier->FindComponentByClass<UOutbreakHealthComponent>();
		if ((!Health || Health->IsAlive()) && FVector::Dist(Soldier->GetActorLocation(), ZoneLocation) <= Radius)
		{
			Extracted.Add(Soldier);
		}
	}
	Mission.RecordExtracted(Extracted);
	return EOutbreakObjectiveState::Completed;
}

void UOutbreakExtractionObjective::Deactivate()
{
	Marker.Reset();
}

bool UOutbreakExtractionObjective::Validate(TArray<FText>& OutErrors) const
{
	if (MarkerId.IsNone())
	{
		OutErrors.Add(LOCTEXT("ExtractionNoMarker", "Extraction objective has no MarkerId."));
		return false;
	}
	return true;
}

FText UOutbreakExtractionObjective::GetProgressText() const
{
	if (RequiredCount > 0 && InZoneCount == RequiredCount)
	{
		return FText::Format(LOCTEXT("ExtractionCountdown", "extracting in {0}"),
			OutbreakObjectiveQueries::FormatTime(ExtractionTime - HeldTime));
	}
	if (InZoneCount > 0)
	{
		return FText::Format(LOCTEXT("ExtractionWaiting", "{0} / {1} in zone"), InZoneCount, RequiredCount);
	}
	return ClosestPlayerDistance >= 0.0f
		? FText::Format(LOCTEXT("ExtractionDistance", "{0} m"), FMath::RoundToInt(ClosestPlayerDistance / 100.0f))
		: FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
