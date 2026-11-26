#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "OutbreakEnemyCoordinationSubsystem.generated.h"

class AOutbreakEnemyAIController;

/** A coordinating enemy's flanking role against its target. */
struct FOutbreakFlankAssignment
{
	/** INDEX_NONE = pin (engage from the front); 0, 1, ... = flank order. */
	int32 FlankOrder = INDEX_NONE;

	/** Where the pinning ally stands (valid when FlankOrder != INDEX_NONE). */
	FVector PinLocation = FVector::ZeroVector;

	bool IsFlanking() const { return FlankOrder != INDEX_NONE; }
};

/**
 * Combat coordination between nearby enemies (Phase 9, server only). Each enemy in combat reports
 * its engagement (who it fights, where it stands, when it last saw the target) at its decision
 * update. Enemies within one another's coordination radius then:
 * - share sightings: an enemy that lost sight of its target keeps tracking it while an ally sees it;
 * - split roles: one pins the target from the front, others flank (OutbreakEnemyCoordination);
 * - spread targets: target scores drop per ally already fighting the target.
 *
 * Entries are refreshed by their owners and expire when not refreshed (dead, disengaged, destroyed).
 */
UCLASS()
class OUTBREAK_API UOutbreakEnemyCoordinationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/** Records or refreshes Enemy's engagement. A null Target removes it. */
	void ReportEngagement(AOutbreakEnemyAIController* Enemy, AActor* Target, const FVector& EnemyLocation, bool bCanFlank,
		bool bTargetSeen, const FVector& SeenTargetLocation);

	void RemoveEngagement(const AOutbreakEnemyAIController* Enemy);

	/** The actor Enemy is reported as fighting, if any. */
	AActor* GetEngagedTarget(const AOutbreakEnemyAIController* Enemy) const;

	/** Allies within Radius of Location fighting Target, not counting Exclude. */
	int32 CountAlliesEngaging(const AActor* Target, const FVector& Location, float Radius,
		const AOutbreakEnemyAIController* Exclude) const;

	/** Allies within Radius of Location in combat (any target), Exclude not counted. */
	int32 CountAlliesInCombat(const FVector& Location, float Radius, const AOutbreakEnemyAIController* Exclude) const;

	/** The most recent sighting of Target by an ally within Radius of Location. False if none. */
	bool GetLatestSighting(const AActor* Target, const FVector& Location, float Radius,
		const AOutbreakEnemyAIController* Exclude, FVector& OutLocation, double& OutTime) const;

	/**
	 * Enemy's role against its reported target among the allies within Radius of it fighting the
	 * same target, at most MaxFlankers flanking.
	 */
	FOutbreakFlankAssignment GetFlankAssignment(const AOutbreakEnemyAIController* Enemy, float Radius, int32 MaxFlankers);

	/** Engagements not refreshed for this long are dropped. */
	static constexpr double EngagementLifetime = 2.0;

private:

	struct FEngagement
	{
		TWeakObjectPtr<AOutbreakEnemyAIController> Enemy;
		TWeakObjectPtr<AActor> Target;
		FVector EnemyLocation = FVector::ZeroVector;
		bool bCanFlank = false;
		bool bWasFlanking = false;
		double UpdateTime = 0.0;
		FVector SightingLocation = FVector::ZeroVector;
		double SightingTime = -1.0e9;
	};

	void PruneExpired();
	double GetTimeSeconds() const;
	const FEngagement* FindEngagement(const AOutbreakEnemyAIController* Enemy) const;

	TArray<FEngagement> Engagements;
};
