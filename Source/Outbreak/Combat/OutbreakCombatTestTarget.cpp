#include "Combat/OutbreakCombatTestTarget.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Outbreak.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AOutbreakCombatTestTarget::AOutbreakCombatTestTarget()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	// Roughly human-sized pillar from the 100 cm engine cube.
	Mesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.8f));
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	HealthComponent = CreateDefaultSubobject<UOutbreakHealthComponent>(TEXT("HealthComponent"));

	PerceptionStimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuliSource"));
}

FGenericTeamId AOutbreakCombatTestTarget::GetGenericTeamId() const
{
	return OutbreakTeam::ToGenericTeamId(Team);
}

void AOutbreakCombatTestTarget::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	HealthComponent->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::HandleHealthChanged);
	HealthComponent->OnHealthStateChanged.AddUniqueDynamic(this, &ThisClass::HandleHealthStateChanged);
}

void AOutbreakCombatTestTarget::BeginPlay()
{
	Super::BeginPlay();

	BaseMeshScale = Mesh->GetRelativeScale3D();

	// Perception runs on the server.
	if (HasAuthority())
	{
		PerceptionStimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
	}
}

void AOutbreakCombatTestTarget::HandleHealthChanged(UOutbreakHealthComponent* InHealthComponent, float NewHealth, float Delta)
{
	if (Delta >= 0.0f)
	{
		return;
	}

	Mesh->SetRelativeScale3D(BaseMeshScale * HitReactionScale);
	GetWorldTimerManager().SetTimer(HitReactionTimer, this, &ThisClass::EndHitReaction, HitReactionDuration, false);

	OnHitReaction(-Delta);
}

void AOutbreakCombatTestTarget::EndHitReaction()
{
	Mesh->SetRelativeScale3D(BaseMeshScale);
}

void AOutbreakCombatTestTarget::HandleHealthStateChanged(UOutbreakHealthComponent* InHealthComponent,
	EOutbreakHealthState NewState, EOutbreakHealthState OldState)
{
	const bool bAlive = NewState == EOutbreakHealthState::Alive;
	SetActorHiddenInGame(!bAlive);
	SetActorEnableCollision(bAlive);

	if (!bAlive && HasAuthority() && RespawnDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &ThisClass::Respawn, RespawnDelay, false);
	}
}

void AOutbreakCombatTestTarget::Respawn()
{
	HealthComponent->ResetHealth();
}
