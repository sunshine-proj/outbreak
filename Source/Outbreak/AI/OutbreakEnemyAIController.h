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
 * - Perception: sight and hearing as separate senses, configured from the enemy definition.
 *   Hearing receives weapon fire noise (UAISense_Hearing, reported by the weapon component).
 * - Target tracking and prioritisation: visible living hostiles scored by distance, recent
 *   attacker and player bias (OutbreakEnemyTargeting::ScoreTarget).
 * - States: Idle / Patrol / Alerted / Combat (OutbreakEnemyBehavior::SelectState). Noise, damage
 *   from an unseen attacker, or losing the target raise an alert at that location.
 * - Positioning: keep within the definition's combat range band (OutbreakEnemyTactics).
 * - Combat: the shared UBTService_OutbreakCombat through IOutbreakAIAgent.
 *
 * Debug drawing: console variable `outbreak.AI.DebugEnemies 1` draws sight range, state and
 * destination at each decision update.
 */
UCLASS()
class OUTBREAK_API AOutbreakEnemyAIController : public AAIController, public IOutbreakAIAgent
{
	GENERATED_BODY()

public:

	AOutbreakEnemyAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin IOutbreakAIAgent interface
	virtual void UpdateBehavior() override;
	virtual AActor* GetCombatTarget() const override { return CurrentTarget.Get(); }
	virtual bool IsActorVisible(const AActor* Actor) const override;
	virtual float GetEngagementRange() const override;
	virtual float GetAimTolerance() const override;
	virtual float GetIdleReloadFraction() const override;
	virtual UOutbreakWeaponComponent* GetAgentWeaponComponent() const override { return WeaponComponent; }
	//~ End IOutbreakAIAgent interface

	/** Raises (or refreshes) an alert at Location: the enemy goes to look. */
	void ReportAlert(const FVector& Location);

	EOutbreakEnemyState GetCurrentState() const { return CurrentState; }
	FVector GetMoveLocation() const { return MoveLocation; }
	bool HasAlert() const { return bHasAlert; }
	FVector GetAlertLocation() const { return AlertLocation; }
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
	void UpdateTargets();
	FVector UpdatePatrol();
	FVector ComputeMoveLocation();
	void WriteBlackboard() const;
	void DrawDebug() const;
	double GetTimeSeconds() const;

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

	TArray<TWeakObjectPtr<AActor>> VisibleHostiles;
	TWeakObjectPtr<AActor> CurrentTarget;
	FVector LastSeenTargetLocation = FVector::ZeroVector;

	TWeakObjectPtr<AActor> LastAttacker;
	double LastAttackTime = -1.0e9;

	bool bHasAlert = false;
	FVector AlertLocation = FVector::ZeroVector;
	double AlertTime = -1.0e9;

	FVector HomeLocation = FVector::ZeroVector;
	int32 PatrolIndex = 0;
	int32 PatrolDirection = 1;
	double PatrolArrivalTime = -1.0;

	EOutbreakEnemyState CurrentState = EOutbreakEnemyState::Idle;
	FVector MoveLocation = FVector::ZeroVector;
	bool bUpdatingBehavior = false;
};
