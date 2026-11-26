#pragma once

#include "CoreMinimal.h"
#include "Combat/OutbreakDamageApplication.h"
#include "GameFramework/Actor.h"

#include "OutbreakProjectile.generated.h"

class UOutbreakAmmoTypeDefinition;
class UProjectileMovementComponent;
class USphereComponent;

/**
 * Replicated projectile spawned by UOutbreakProjectileFireBehavior.
 *
 * Authority applies damage on impact and destroys the projectile. Every machine plays the impact
 * effects when its copy stops. A client whose copy is destroyed by replication before it stops
 * shows no impact effect (accepted for the prototype).
 */
UCLASS(Blueprintable)
class OUTBREAK_API AOutbreakProjectile : public AActor
{
	GENERATED_BODY()

public:

	AOutbreakProjectile();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Call between SpawnActorDeferred and FinishSpawning. */
	void InitializeProjectile(const FOutbreakImpactDamage& InImpactDamage, const UOutbreakAmmoTypeDefinition* InAmmoType,
		float InitialSpeed, float GravityScale);

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

private:

	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);

	/** Replicated so clients can play the ammunition's impact effects. */
	UPROPERTY(Replicated)
	TObjectPtr<UOutbreakAmmoTypeDefinition> AmmoType;

	/** Authority only. */
	FOutbreakImpactDamage ImpactDamage;
};
