#pragma once

#include "CoreMinimal.h"
#include "Combat/OutbreakDamageApplication.h"
#include "Engine/EngineTypes.h"
#include "UObject/Object.h"
#include "Weapons/OutbreakWeaponTypes.h"

#include "OutbreakFireBehavior.generated.h"

class AController;
class AOutbreakProjectile;
class APawn;
class UOutbreakAmmoTypeDefinition;

/** Everything a fire behaviour needs to deliver one validated shot. Built by the weapon component. */
struct FOutbreakShotRequest
{
	UWorld* World = nullptr;
	APawn* InstigatorPawn = nullptr;
	AController* InstigatorController = nullptr;
	AActor* DamageCauser = nullptr;
	const UOutbreakAmmoTypeDefinition* AmmoType = nullptr;

	FVector Origin = FVector::ZeroVector;

	/** One normalized direction per projectile or trace, spread already applied. */
	TArray<FVector> Directions;

	float Range = 0.0f;
	FOutbreakImpactDamage ImpactDamage;
};

/**
 * How a fire mode delivers a shot. Instanced inside a fire mode of UOutbreakWeaponDefinition.
 *
 * Runs on the authority only, after the weapon component has validated the shot and consumed
 * ammunition. Implementations must be stateless: one instance is shared by every user of the
 * weapon definition.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories, Const)
class OUTBREAK_API UOutbreakFireBehavior : public UObject
{
	GENERATED_BODY()

public:

	/** Delivers the shot. Appends an impact for each trace that cosmetic effects should show. */
	virtual void ExecuteShot(const FOutbreakShotRequest& Request, TArray<FOutbreakShotImpact>& OutImpacts) const
		PURE_VIRTUAL(UOutbreakFireBehavior::ExecuteShot, );
};

/** Instant line traces. Damage is applied to whatever each trace hits first. */
UCLASS(DisplayName = "Hitscan")
class OUTBREAK_API UOutbreakHitscanFireBehavior : public UOutbreakFireBehavior
{
	GENERATED_BODY()

public:

	virtual void ExecuteShot(const FOutbreakShotRequest& Request, TArray<FOutbreakShotImpact>& OutImpacts) const override;

	UPROPERTY(EditAnywhere, Category = "Hitscan")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

/** Spawns a replicated projectile per direction. The projectile applies damage on impact. */
UCLASS(DisplayName = "Projectile")
class OUTBREAK_API UOutbreakProjectileFireBehavior : public UOutbreakFireBehavior
{
	GENERATED_BODY()

public:

	UOutbreakProjectileFireBehavior();

	virtual void ExecuteShot(const FOutbreakShotRequest& Request, TArray<FOutbreakShotImpact>& OutImpacts) const override;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	TSubclassOf<AOutbreakProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (ClampMin = "1.0", Units = "cm/s"))
	float InitialSpeed = 5000.0f;

	/** 0 = flies straight. */
	UPROPERTY(EditAnywhere, Category = "Projectile")
	float GravityScale = 0.0f;
};
