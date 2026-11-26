#include "Missions/OutbreakDestructibleObject.h"

#include "Combat/OutbreakDamageApplication.h"
#include "Combat/OutbreakDamageTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Stealth/OutbreakNoiseTypes.h"
#include "UObject/ConstructorHelpers.h"

AOutbreakDestructibleObject::AOutbreakDestructibleObject()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	RootComponent = Mesh;

	// Greybox default: the engine cube. Designers replace it in a Blueprint or per instance.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	HealthComponent = CreateDefaultSubobject<UOutbreakHealthComponent>(TEXT("HealthComponent"));
}

void AOutbreakDestructibleObject::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	HealthComponent->OnHealthStateChanged.AddUniqueDynamic(this, &ThisClass::HandleHealthStateChanged);
	HealthComponent->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::HandleHealthChanged);
	OnTakeAnyDamage.AddUniqueDynamic(this, &ThisClass::HandleTakeAnyDamage);
}

bool AOutbreakDestructibleObject::IsDestroyed() const
{
	return HealthComponent->GetHealthState() == EOutbreakHealthState::Dead;
}

bool AOutbreakDestructibleObject::IsDamaged() const
{
	return IsDestroyed() || HealthComponent->GetHealthFraction() <= DamagedHealthFraction;
}

void AOutbreakDestructibleObject::SetExplosion(float Damage, float Radius, TSubclassOf<UDamageType> DamageType)
{
	if (HasAuthority())
	{
		ExplosionDamage = FMath::Max(0.0f, Damage);
		ExplosionRadius = FMath::Max(0.0f, Radius);
		ExplosionDamageType = DamageType;
	}
}

void AOutbreakDestructibleObject::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if (InstigatedBy)
	{
		LastInstigator = InstigatedBy;
	}
}

void AOutbreakDestructibleObject::HandleHealthChanged(UOutbreakHealthComponent* InHealthComponent, float NewHealth, float Delta)
{
	UpdateDamagedStage();
}

void AOutbreakDestructibleObject::UpdateDamagedStage()
{
	if (!DamagedMesh || IsDestroyed() || bShowingDamaged || !IsDamaged())
	{
		return;
	}
	bShowingDamaged = true;
	Mesh->SetStaticMesh(DamagedMesh);
}

void AOutbreakDestructibleObject::HandleHealthStateChanged(UOutbreakHealthComponent* InHealthComponent,
	EOutbreakHealthState NewState, EOutbreakHealthState OldState)
{
	if (NewState != EOutbreakHealthState::Dead)
	{
		return;
	}

	if (DestroyedMesh)
	{
		Mesh->SetStaticMesh(DestroyedMesh);
	}
	else
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}

	// Cosmetics on every machine (health state replicates).
	if (DestructionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, DestructionEffect, GetActorLocation(), GetActorRotation());
	}
	if (DestructionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DestructionSound, GetActorLocation());
	}

	if (HasAuthority())
	{
		if (ExplosionDamage > 0.0f && ExplosionRadius > 0.0f)
		{
			Explode();
		}
		else if (DestructionNoiseLoudness > 0.0f)
		{
			FOutbreakNoiseEvent Noise;
			Noise.Type = EOutbreakNoiseType::Impact;
			Noise.Location = GetActorLocation();
			Noise.Loudness = DestructionNoiseLoudness;
			Noise.Instigator = LastInstigator.IsValid() ? LastInstigator->GetPawn() : nullptr;
			OutbreakNoise::ReportNoise(GetWorld(), Noise);
		}
	}
	OnObjectDestroyed();
}

void AOutbreakDestructibleObject::Explode()
{
	FHitResult Hit;
	Hit.Location = GetActorLocation();
	Hit.ImpactPoint = GetActorLocation();

	FOutbreakImpactDamage Damage;
	Damage.Damage = ExplosionDamage;
	Damage.ExplosionRadius = ExplosionRadius;
	Damage.DamageType = ExplosionDamageType ? ExplosionDamageType : TSubclassOf<UDamageType>(UOutbreakExplosiveDamageType::StaticClass());

	// Reports the explosion noise too. This object is dead, so its own blast does nothing to it;
	// other destructibles in range can go off in turn.
	OutbreakDamage::ApplyImpactDamage(Hit, FVector::UpVector, Damage, LastInstigator.Get(), this);
}
