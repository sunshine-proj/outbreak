#include "Inventory/OutbreakItemAction.h"

#include "Combat/OutbreakDamageApplication.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakProjectile.h"

bool UOutbreakHealItemAction::CanUse(const FOutbreakItemUseContext& Context) const
{
	if (!Context.User || !Context.Target || (!bCanUseOnOthers && Context.Target != Context.User))
	{
		return false;
	}
	const UOutbreakHealthComponent* Health = Context.Target->FindComponentByClass<UOutbreakHealthComponent>();
	return Health && Health->IsAlive() && Health->GetHealth() < Health->GetMaxHealth();
}

bool UOutbreakHealItemAction::Use(const FOutbreakItemUseContext& Context) const
{
	if (!CanUse(Context))
	{
		return false;
	}
	UOutbreakHealthComponent* Health = Context.Target->FindComponentByClass<UOutbreakHealthComponent>();
	return Health->Heal(HealAmount * FMath::Max(0.0f, Context.EffectMultiplier)) > 0.0f;
}

UOutbreakThrowItemAction::UOutbreakThrowItemAction()
{
	ProjectileClass = AOutbreakProjectile::StaticClass();
}

bool UOutbreakThrowItemAction::Use(const FOutbreakItemUseContext& Context) const
{
	APawn* UserPawn = Cast<APawn>(Context.User);
	UWorld* World = UserPawn ? UserPawn->GetWorld() : nullptr;
	if (!World || !ProjectileClass)
	{
		return false;
	}

	FVector EyeLocation;
	FRotator EyeRotation;
	UserPawn->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	EyeRotation.Pitch = FMath::Clamp(EyeRotation.Pitch + LaunchPitchOffset, -89.0f, 89.0f);
	const FVector Direction = EyeRotation.Vector();
	// Start a little ahead of the eyes so the throw clears the thrower's own capsule.
	const FTransform SpawnTransform(EyeRotation, EyeLocation + Direction * 60.0f);

	AOutbreakProjectile* Projectile = World->SpawnActorDeferred<AOutbreakProjectile>(ProjectileClass, SpawnTransform,
		UserPawn, UserPawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return false;
	}

	FOutbreakImpactDamage ImpactDamage;
	ImpactDamage.Damage = Damage * FMath::Max(0.0f, Context.EffectMultiplier);
	ImpactDamage.DamageType = AmmoType ? AmmoType->DamageType : nullptr;
	ImpactDamage.ExplosionRadius = AmmoType ? AmmoType->ExplosionRadius : 0.0f;
	Projectile->InitializeProjectile(ImpactDamage, AmmoType, ThrowSpeed, GravityScale);
	Projectile->FinishSpawning(SpawnTransform);
	return true;
}
