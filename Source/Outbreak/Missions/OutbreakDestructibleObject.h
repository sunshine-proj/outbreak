#pragma once

#include "CoreMinimal.h"
#include "Combat/OutbreakHealthComponent.h"
#include "GameFramework/Actor.h"

#include "OutbreakDestructibleObject.generated.h"

class AController;
class UDamageType;
class UNiagaraSystem;
class USoundBase;
class UStaticMeshComponent;
class UStaticMesh;

/**
 * A damageable level object (Phase 11 Destroy Object targets, Phase 12 basic destruction): a relay,
 * a fuel tank, an alien nest, a crate used as cover.
 *
 * Takes damage through the engine pipeline like any actor with a health component. Its health
 * component's Damage Multipliers decide what can hurt it (for example only explosives).
 * - Damaged: below DamagedHealthFraction it switches to DamagedMesh.
 * - Destroyed: it switches to DestroyedMesh (rubble that still collides), or hides and stops
 *   colliding when there is none, so cover behind it is gone. Destruction is loud (a noise enemies
 *   hear), plays DestructionEffect and DestructionSound, and with ExplosionDamage > 0 it explodes,
 *   which can set off other destructibles.
 * It has no team, so AI does not target it on its own. Health replicates, so every machine shows
 * the same stage.
 */
UCLASS(Blueprintable)
class OUTBREAK_API AOutbreakDestructibleObject : public AActor
{
	GENERATED_BODY()

public:

	AOutbreakDestructibleObject();

	UOutbreakHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "Destructible")
	bool IsDestroyed() const;

	/** Below the damaged threshold (or destroyed). */
	UFUNCTION(BlueprintPure, Category = "Destructible")
	bool IsDamaged() const;

	/** Authority: makes it explode when destroyed. Damage 0 = no explosion. */
	void SetExplosion(float Damage, float Radius, TSubclassOf<UDamageType> DamageType = nullptr);

protected:

	virtual void PostInitializeComponents() override;

	/** Called on every machine when the object is destroyed, for effects. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Destructible")
	void OnObjectDestroyed();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UOutbreakHealthComponent> HealthComponent;

	/** Shown once destroyed (wreckage). Empty = the object disappears. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destructible")
	TObjectPtr<UStaticMesh> DestroyedMesh;

	/** Shown below DamagedHealthFraction. Empty = no damaged stage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destructible")
	TObjectPtr<UStaticMesh> DamagedMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destructible", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamagedHealthFraction = 0.5f;

	/** Gameplay noise of being destroyed (an explosion makes its own, louder noise). 0 = silent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destructible", meta = (ClampMin = "0.0"))
	float DestructionNoiseLoudness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destructible|Effects")
	TObjectPtr<UNiagaraSystem> DestructionEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destructible|Effects")
	TObjectPtr<USoundBase> DestructionSound;

	/** Radial damage when destroyed (a fuel tank). 0 = does not explode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destructible|Explosion", meta = (ClampMin = "0.0"))
	float ExplosionDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destructible|Explosion", meta = (ClampMin = "0.0", Units = "cm"))
	float ExplosionRadius = 500.0f;

	/** Empty = UOutbreakExplosiveDamageType. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destructible|Explosion")
	TSubclassOf<UDamageType> ExplosionDamageType;

private:

	UFUNCTION()
	void HandleHealthStateChanged(UOutbreakHealthComponent* InHealthComponent, EOutbreakHealthState NewState, EOutbreakHealthState OldState);

	UFUNCTION()
	void HandleHealthChanged(UOutbreakHealthComponent* InHealthComponent, float NewHealth, float Delta);

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	void UpdateDamagedStage();
	void Explode();

	/** Who last damaged it, credited with the explosion. */
	TWeakObjectPtr<AController> LastInstigator;
	bool bShowingDamaged = false;
};
