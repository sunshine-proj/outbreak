#pragma once

#include "CoreMinimal.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Combat/OutbreakTeam.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"

#include "OutbreakCombatTestTarget.generated.h"

class UAIPerceptionStimuliSourceComponent;
class UStaticMeshComponent;

/**
 * Greybox combat target: a damageable pillar with a team.
 *
 * With Team = Hostile (the default) it is the hostile test target used in Phase 4 to check Attack
 * orders and threat response before enemy AI exists. It registers as an AI sight stimulus. With
 * Team = Neutral it is a plain damageable target. Not intended as shipping content.
 */
UCLASS(Blueprintable)
class OUTBREAK_API AOutbreakCombatTestTarget : public AActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:

	AOutbreakCombatTestTarget();

	//~ Begin IGenericTeamAgentInterface interface
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~ End IGenericTeamAgentInterface interface

	UOutbreakHealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UOutbreakHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> PerceptionStimuliSource;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team")
	EOutbreakTeam Team = EOutbreakTeam::Hostile;

	/** Seconds after death before the target restores itself. 0 = stays dead. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Test Target", meta = (ClampMin = "0.0", Units = "s"))
	float RespawnDelay = 3.0f;

	/** Scale applied briefly when hit, as a visible hit reaction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Test Target", meta = (ClampMin = "0.1"))
	float HitReactionScale = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Test Target", meta = (ClampMin = "0.01", Units = "s"))
	float HitReactionDuration = 0.1f;

	/** Called on every machine when the target takes damage, for Blueprint feedback. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Test Target")
	void OnHitReaction(float Damage);

private:

	UFUNCTION()
	void HandleHealthChanged(UOutbreakHealthComponent* InHealthComponent, float NewHealth, float Delta);

	UFUNCTION()
	void HandleHealthStateChanged(UOutbreakHealthComponent* InHealthComponent, EOutbreakHealthState NewState, EOutbreakHealthState OldState);

	void EndHitReaction();
	void Respawn();

	FVector BaseMeshScale = FVector::OneVector;
	FTimerHandle HitReactionTimer;
	FTimerHandle RespawnTimer;
};
