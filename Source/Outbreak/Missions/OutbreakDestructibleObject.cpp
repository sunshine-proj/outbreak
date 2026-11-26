#include "Missions/OutbreakDestructibleObject.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
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
}

bool AOutbreakDestructibleObject::IsDestroyed() const
{
	return HealthComponent->GetHealthState() == EOutbreakHealthState::Dead;
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
	OnObjectDestroyed();
}
