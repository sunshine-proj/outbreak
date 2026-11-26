#pragma once

#include "CoreMinimal.h"
#include "AI/OutbreakEnemyTypes.h"
#include "Characters/OutbreakCharacter.h"

#include "OutbreakEnemyCharacter.generated.h"

class AOutbreakPatrolRoute;
class UOutbreakEnemyDefinition;

/**
 * Base enemy. Its archetype (health, movement, weapon, perception, behaviour) comes from a
 * UOutbreakEnemyDefinition, so new enemies are data, not code. Driven by AOutbreakEnemyAIController.
 *
 * Animation integration: the replicated EnemyState plus IsDead / IsInCombat let an Animation
 * Blueprint pick poses, and OnEnemyStateChanged / OnDowned notify Blueprints of transitions.
 * (Enemies inherit a squad member component from AOutbreakCharacter; it stays unused.)
 */
UCLASS(Blueprintable)
class OUTBREAK_API AOutbreakEnemyCharacter : public AOutbreakCharacter
{
	GENERATED_BODY()

public:

	AOutbreakEnemyCharacter();

	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	UOutbreakEnemyDefinition* GetEnemyDefinition() const { return EnemyDefinition; }

	/** Call before spawning finishes (SpawnActorDeferred) so it is applied on initialization. */
	void SetEnemyDefinition(UOutbreakEnemyDefinition* NewDefinition) { EnemyDefinition = NewDefinition; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	AOutbreakPatrolRoute* GetPatrolRoute() const { return PatrolRoute; }

	void SetPatrolRoute(AOutbreakPatrolRoute* NewRoute) { PatrolRoute = NewRoute; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Animation")
	EOutbreakEnemyState GetEnemyState() const { return EnemyState; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Animation")
	bool IsInCombat() const { return EnemyState == EOutbreakEnemyState::Combat || EnemyState == EOutbreakEnemyState::Retreating; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Animation")
	bool IsDead() const;

	/** Authority; set by the AI controller. */
	void SetEnemyState(EOutbreakEnemyState NewState);

protected:

	virtual void BeginPlay() override;
	virtual void HandleDowned(EOutbreakHealthState NewState) override;

	/** Called on every machine when the AI state changes, for animation and audio Blueprints. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Animation")
	void OnEnemyStateChanged(EOutbreakEnemyState NewState, EOutbreakEnemyState OldState);

	/** The archetype. Can be overridden per placed enemy. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UOutbreakEnemyDefinition> EnemyDefinition;

	/** Route to patrol. Empty = stand at the spawn position when idle. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<AOutbreakPatrolRoute> PatrolRoute;

private:

	void ApplyDefinition();

	/** Every machine: barks (Phase 13). Authority: reports combat to the alarm. */
	void HandleEnemyStateChanged(EOutbreakEnemyState NewState, EOutbreakEnemyState OldState);

	double LastBarkTime = -1.0e9;

	UFUNCTION()
	void OnRep_EnemyState(EOutbreakEnemyState OldState);

	UPROPERTY(ReplicatedUsing = OnRep_EnemyState)
	EOutbreakEnemyState EnemyState = EOutbreakEnemyState::Idle;
};
