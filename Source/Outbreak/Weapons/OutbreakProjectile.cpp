#include "Weapons/OutbreakProjectile.h"

#include "Components/SphereComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakWeaponEffects.h"

AOutbreakProjectile::AOutbreakProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	InitialLifeSpan = 5.0f;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(5.0f);
	CollisionComponent->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	CollisionComponent->CanCharacterStepUpOn = ECB_No;
	RootComponent = CollisionComponent;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 5000.0f;
	ProjectileMovement->MaxSpeed = 0.0f; // No limit.
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
}

void AOutbreakProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AOutbreakProjectile, AmmoType, COND_InitialOnly);
}

void AOutbreakProjectile::InitializeProjectile(const FOutbreakImpactDamage& InImpactDamage,
	const UOutbreakAmmoTypeDefinition* InAmmoType, float InitialSpeed, float GravityScale)
{
	ImpactDamage = InImpactDamage;
	// Definitions are immutable assets; the pointer is non-const only because it is a replicated UPROPERTY.
	AmmoType = const_cast<UOutbreakAmmoTypeDefinition*>(InAmmoType);
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->ProjectileGravityScale = GravityScale;
}

void AOutbreakProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* InstigatorActor = GetInstigator())
	{
		CollisionComponent->IgnoreActorWhenMoving(InstigatorActor, true);
	}

	ProjectileMovement->OnProjectileStop.AddUniqueDynamic(this, &ThisClass::HandleProjectileStop);
}

void AOutbreakProjectile::HandleProjectileStop(const FHitResult& ImpactResult)
{
	OutbreakWeaponEffects::PlayImpactEffects(this, AmmoType, ImpactResult.ImpactPoint, ImpactResult.ImpactNormal);

	if (HasAuthority())
	{
		OutbreakDamage::ApplyImpactDamage(ImpactResult, GetVelocity().GetSafeNormal(), ImpactDamage,
			GetInstigatorController(), this);
		Destroy();
	}
	else
	{
		// Hide the client copy; the authority destroys it.
		SetActorHiddenInGame(true);
	}
}
