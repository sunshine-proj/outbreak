#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/OutbreakAIAgent.h"
#include "AI/OutbreakEnemyTypes.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Perception/AIPerceptionTypes.h"

#include "OutbreakEnemyAIController.generated.h"

class AOutbreakEnemyCharacter;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
class UOutbreakEnemyDefinition;
class UOutbreakWeaponComponent;
class UDamageType;

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
 * - Positioning: combat range band (OutbreakEnemyTactics); search points on the navmesh.
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

	TWeakObjectPtr<AActor> LastAttacker;
	double LastAttackTime = -1.0e9;

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
