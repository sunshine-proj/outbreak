#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/OutbreakAIAgent.h"
#include "AI/OutbreakEnemyTactics.h"
#include "AI/OutbreakEnemyTypes.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Perception/AIPerceptionTypes.h"

#include "OutbreakEnemyAIController.generated.h"

class AOutbreakEnemyCharacter;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
class UOutbreakEnemyCoordinationSubsystem;
class UOutbreakEnemyDefinition;
class UOutbreakWeaponComponent;
class UDamageType;
struct FEnvQueryResult;

/**
 * AI controller for enemies (server only).
 *
 * - Sight: the engine sight sense finds candidates in range, cone and line of sight; each one's
 *   visibility (light, exposure, movement, distance, field of view, stance) drives a per-target
 *   detection level (OutbreakDetection). Detection 1 = detected.
 * - Hearing: gameplay noises (OutbreakNoise) are attenuated by distance and obstacles and raise
 *   suspicion at the noise location. Being shot raises it to the maximum.
 * - States: Idle / Patrol / Suspicious / Investigating / Combat / Searching
 *   (OutbreakEnemyBehavior::SelectState). Losing a detected target starts a search around its last
 *   known position. Detecting one alerts other enemies nearby.
 * - Tactics (Phase 9, OutbreakEnemyTactics, data in UOutbreakEnemyDefinition::Tactics): in combat,
 *   the movement is Engage (range band / advance), Charge (melee), TakeCover (EQS), Flank (role from
 *   UOutbreakEnemyCoordinationSubsystem) or Retreat. Nearby enemies share sightings, split pin and
 *   flank roles and spread their targets across the squad. Targets switch only for a clearly better
 *   one. Low health or being outnumbered starts a retreat; a leash keeps fights near home and a
 *   target lost beyond it is given up (disengagement).
 * - Search points on the navmesh after losing a target.
 * - Combat: the shared UBTService_OutbreakCombat through IOutbreakAIAgent.
 *
 * Debug drawing: `outbreak.AI.DebugEnemies 1`.
 */
UCLASS()
class OUTBREAK_API AOutbreakEnemyAIController : public AAIController, public IOutbreakAIAgent
{
	GENERATED_BODY()

public:

	AOutbreakEnemyAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin IOutbreakAIAgent interface
	virtual void UpdateBehavior() override;
	/** The detected target, while tracked. */
	virtual AActor* GetCombatTarget() const override { return CurrentTarget.Get(); }
	virtual bool IsActorVisible(const AActor* Actor) const override;
	virtual float GetEngagementRange() const override;
	virtual float GetAimTolerance() const override;
	virtual float GetIdleReloadFraction() const override;
	virtual UOutbreakWeaponComponent* GetAgentWeaponComponent() const override { return WeaponComponent; }
	/** The target (or where it was last seen), else a recent attacker. */
	virtual bool GetCoverThreatLocation(FVector& OutLocation) const override;
	/** The flank position while flanking (cover near it); otherwise the enemy itself (false). */
	virtual bool GetCoverAnchorLocation(FVector& OutLocation) const override;
	//~ End IOutbreakAIAgent interface

	/** Adds SuspicionAmount (0..1) and makes Location the place to investigate. */
	void ReportStimulus(const FVector& Location, float SuspicionAmount);

	/** Full suspicion at Location (a gunshot nearby, being hit by an unseen attacker). */
	void ReportAlert(const FVector& Location) { ReportStimulus(Location, 1.0f); }

	/** Called by another enemy that detected a hostile at Location. */
	void ReceivePropagatedAlert(const FVector& Location);

	/** This enemy's detection of Target, 0..1 (1 while Target is its tracked target). */
	float GetDetectionOf(const AActor* Target) const;

	/** The highest detection of Target by any enemy in World (teammate stealth awareness, HUD). */
	static float GetHighestDetectionOf(const UWorld* World, const AActor* Target);

	/** Current visibility 0..1 of Target from this enemy's eyes (ignores the sight cone and range checks). */
	float ComputeTargetVisibility(const AActor* Target) const;

	EOutbreakEnemyState GetCurrentState() const { return CurrentState; }
	float GetSuspicion() const { return Suspicion; }
	FVector GetMoveLocation() const { return MoveLocation; }
	bool HasStimulus() const { return bHasStimulus; }
	FVector GetStimulusLocation() const { return StimulusLocation; }
	int32 GetPatrolIndex() const { return PatrolIndex; }

	// Phase 9 tactics (read by tests, debugging and Blueprints via the character).
	EOutbreakEnemyCombatMove GetCombatMove() const { return CombatMove; }
	bool IsRetreating() const { return bRetreating; }
	FVector GetRetreatLocation() const { return RetreatLocation; }
	bool HasCover() const { return bHasCover; }
	FVector GetCoverLocation() const { return CoverLocation; }
	/** INDEX_NONE unless flanking. */
	int32 GetFlankOrder() const { return FlankOrder; }
	FVector GetFlankLocation() const { return FlankLocation; }
	FVector GetHomeLocation() const { return HomeLocation; }
	FVector GetLastSeenTargetLocation() const { return LastSeenTargetLocation; }

	/** Sets the cover position as if the cover query had returned it (tests, scripted cover). */
	void SetCoverLocation(const FVector& Location);

protected:

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

private:

	const UOutbreakEnemyDefinition* GetDefinition() const;
	void ApplyPerceptionConfig();
	void UpdateSight(float DeltaSeconds);
	void PropagateAlert(const FVector& Location);
	FVector UpdatePatrol();
	FVector UpdateSearch();
	FVector ComputeMoveLocation(bool bStateChanged);

	// Phase 9 tactics.
	UOutbreakEnemyCoordinationSubsystem* GetCoordination() const;
	void RefreshFromSharedSightings(double Now);
	void LoseTarget(double Now);
	void UpdateRetreat(double Now);
	bool StartRetreat(double Now);
	void UpdateEngagementReport(EOutbreakEnemyState State);
	FVector ComputeCombatMoveLocation(double Now);
	/** Updates the flank role and position. True while this enemy has a reachable flank role. */
	bool UpdateFlank(const FVector& TargetLocation, bool& bOutReached);
	void RequestCoverQueryIfNeeded(double Now);
	void HandleCoverQueryResult(TSharedPtr<FEnvQueryResult> Result);
	void ResetTactics();
	bool ProjectToNavigation(FVector& InOutLocation) const;
	void WriteBlackboard() const;
	void DrawDebug() const;
	double GetTimeSeconds() const;
	static bool IsAlive(const AActor* Actor);

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleHealthStateChanged(UOutbreakHealthComponent* HealthComponent, EOutbreakHealthState NewState, EOutbreakHealthState OldState);

	UPROPERTY(Transient)
	TObjectPtr<AOutbreakEnemyCharacter> Enemy;

	UPROPERTY(Transient)
	TObjectPtr<UOutbreakWeaponComponent> WeaponComponent;

	UPROPERTY(Transient)
	TObjectPtr<UOutbreakHealthComponent> HealthComponent;

	// Sight and detection.
	TMap<TWeakObjectPtr<AActor>, float> Detection;
	TArray<TWeakObjectPtr<AActor>> VisibleHostiles;
	TWeakObjectPtr<AActor> CurrentTarget;
	FVector LastSeenTargetLocation = FVector::ZeroVector;
	double LastSeenTargetTime = -1.0e9;

	/** When this enemy itself last saw its target (shared sightings do not count). */
	double LastOwnSightTime = -1.0e9;

	TWeakObjectPtr<AActor> LastAttacker;
	double LastAttackTime = -1.0e9;

	// Tactics (Phase 9).
	EOutbreakEnemyCombatMove CombatMove = EOutbreakEnemyCombatMove::None;

	FVector CoverLocation = FVector::ZeroVector;
	bool bHasCover = false;
	bool bCoverQueryPending = false;
	double LastCoverQueryTime = -1.0e9;

	int32 FlankOrder = INDEX_NONE;
	int32 FlankSide = 1;
	FVector FlankLocation = FVector::ZeroVector;

	bool bRetreating = false;
	bool bRetreatedForHealth = false;
	double RetreatStartTime = -1.0e9;
	double LastNumbersRetreatTime = -1.0e9;
	FVector RetreatLocation = FVector::ZeroVector;

	// Suspicion and stimuli.
	float Suspicion = 0.0f;
	bool bHasStimulus = false;
	FVector StimulusLocation = FVector::ZeroVector;

	// Search.
	double SearchStartTime = -1.0e9;
	FVector SearchCenter = FVector::ZeroVector;
	FVector SearchPoint = FVector::ZeroVector;
	bool bHasSearchPoint = false;
	double SearchPointArrivalTime = -1.0;

	// Patrol and home.
	FVector HomeLocation = FVector::ZeroVector;
	int32 PatrolIndex = 0;
	int32 PatrolDirection = 1;
	double PatrolArrivalTime = -1.0;

	EOutbreakEnemyState CurrentState = EOutbreakEnemyState::Idle;
	FVector MoveLocation = FVector::ZeroVector;
	double LastUpdateTime = -1.0;
	bool bUpdatingBehavior = false;
};
