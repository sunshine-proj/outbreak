#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

#include "OutbreakDamageApplication.generated.h"

class AActor;
class AController;
class UDamageType;
struct FHitResult;

/** Damage carried by one hitscan hit or one projectile. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakImpactDamage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	float Damage = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	TSubclassOf<UDamageType> DamageType;

	/** 0 = point damage to the hit actor; > 0 = radial damage around the impact point. */
	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	float ExplosionRadius = 0.0f;
};

namespace OutbreakDamage
{
	/**
	 * Applies ImpactDamage for a hit through the engine damage pipeline: point damage to the hit
	 * actor, or radial damage (with falloff) when ExplosionRadius > 0. Authority only.
	 */
	OUTBREAK_API void ApplyImpactDamage(const FHitResult& Hit, const FVector& ShotDirection,
		const FOutbreakImpactDamage& ImpactDamage, AController* InstigatorController, AActor* DamageCauser);
}
