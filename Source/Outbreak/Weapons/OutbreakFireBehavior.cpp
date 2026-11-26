#include "Weapons/OutbreakFireBehavior.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Outbreak.h"
#include "Weapons/OutbreakProjectile.h"

void UOutbreakHitscanFireBehavior::ExecuteShot(const FOutbreakShotRequest& Request, TArray<FOutbreakShotImpact>& OutImpacts) const
{
	if (!Request.World)
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OutbreakHitscan), /*bTraceComplex=*/ true, Request.InstigatorPawn);
	QueryParams.bReturnPhysicalMaterial = true;
	if (Request.DamageCauser && Request.DamageCauser != Request.InstigatorPawn)
	{
		QueryParams.AddIgnoredActor(Request.DamageCauser);
	}

	for (const FVector& Direction : Request.Directions)
	{
		const FVector TraceEnd = Request.Origin + Direction * Request.Range;

		FHitResult Hit;
		const bool bHit = Request.World->LineTraceSingleByChannel(Hit, Request.Origin, TraceEnd, TraceChannel, QueryParams);

		FOutbreakShotImpact& Impact = OutImpacts.AddDefaulted_GetRef();
		Impact.bBlockingHit = bHit;
		Impact.Location = bHit ? Hit.ImpactPoint : TraceEnd;
		Impact.Normal = bHit ? Hit.ImpactNormal : -Direction;

		if (bHit)
		{
			OutbreakDamage::ApplyImpactDamage(Hit, Direction, Request.ImpactDamage,
				Request.InstigatorController, Request.DamageCauser);
		}
	}
}

UOutbreakProjectileFireBehavior::UOutbreakProjectileFireBehavior()
{
	ProjectileClass = AOutbreakProjectile::StaticClass();
}

void UOutbreakProjectileFireBehavior::ExecuteShot(const FOutbreakShotRequest& Request, TArray<FOutbreakShotImpact>& OutImpacts) const
{
	if (!Request.World || !ProjectileClass)
	{
		UE_LOG(LogOutbreak, Warning, TEXT("%s: cannot fire, no world or ProjectileClass."), *GetName());
		return;
	}

	for (const FVector& Direction : Request.Directions)
	{
		const FTransform SpawnTransform(Direction.Rotation(), Request.Origin);
		AOutbreakProjectile* Projectile = Request.World->SpawnActorDeferred<AOutbreakProjectile>(
			ProjectileClass, SpawnTransform, Request.DamageCauser, Request.InstigatorPawn,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (Projectile)
		{
			Projectile->InitializeProjectile(Request.ImpactDamage, Request.AmmoType, InitialSpeed, GravityScale);
			Projectile->FinishSpawning(SpawnTransform);
		}
	}
}

void UOutbreakMeleeFireBehavior::ExecuteShot(const FOutbreakShotRequest& Request, TArray<FOutbreakShotImpact>& OutImpacts) const
{
	if (!Request.World)
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OutbreakMelee), /*bTraceComplex=*/ false, Request.InstigatorPawn);
	if (Request.DamageCauser && Request.DamageCauser != Request.InstigatorPawn)
	{
		QueryParams.AddIgnoredActor(Request.DamageCauser);
	}

	for (const FVector& Direction : Request.Directions)
	{
		const FVector SweepEnd = Request.Origin + Direction * Request.Range;

		FHitResult Hit;
		const bool bHit = Request.World->SweepSingleByChannel(Hit, Request.Origin, SweepEnd, FQuat::Identity,
			TraceChannel, FCollisionShape::MakeSphere(SweepRadius), QueryParams);

		FOutbreakShotImpact& Impact = OutImpacts.AddDefaulted_GetRef();
		Impact.bBlockingHit = bHit;
		Impact.Location = bHit ? Hit.ImpactPoint : SweepEnd;
		Impact.Normal = bHit ? Hit.ImpactNormal : -Direction;

		if (bHit)
		{
			OutbreakDamage::ApplyImpactDamage(Hit, Direction, Request.ImpactDamage,
				Request.InstigatorController, Request.DamageCauser);
		}
	}
}
