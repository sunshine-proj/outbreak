#include "Characters/OutbreakEnemyCharacter.h"

#include "AI/OutbreakEnemyAIController.h"
#include "AI/OutbreakEnemyDefinition.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Outbreak.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

AOutbreakEnemyCharacter::AOutbreakEnemyCharacter()
{
	AIControllerClass = AOutbreakEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	Team = EOutbreakTeam::Hostile;

	// Turn smoothly towards the controller's focus (target, or movement direction).
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	bUseControllerRotationYaw = false;
	Movement->bUseControllerDesiredRotation = true;
	Movement->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
}

void AOutbreakEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOutbreakEnemyCharacter, EnemyState);
}

void AOutbreakEnemyCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ApplyDefinition();
}

void AOutbreakEnemyCharacter::ApplyDefinition()
{
	if (!EnemyDefinition)
	{
		UE_LOG(LogOutbreak, Warning, TEXT("%s: no EnemyDefinition; using class defaults."), *GetName());
		return;
	}

	// Movement applies on every machine (client movement prediction needs the same speeds).
	SetMovementSettings(EnemyDefinition->MovementSettings);

	if (HasAuthority())
	{
		HealthComponent->SetMaxHealth(EnemyDefinition->MaxHealth, /*bRefillHealth=*/ true);
		for (const FOutbreakDamageMultiplier& Entry : EnemyDefinition->DamageMultipliers)
		{
			HealthComponent->SetDamageMultiplier(Entry.DamageType, Entry.Multiplier);
		}
	}
}

void AOutbreakEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	UOutbreakWeaponDefinition* Weapon = EnemyDefinition ? EnemyDefinition->Weapon.Get() : nullptr;
	if (HasAuthority() && Weapon)
	{
		if (!Weapon->CompatibleAmmoTypes.IsEmpty())
		{
			WeaponComponent->AddAmmo(Weapon->CompatibleAmmoTypes[0], EnemyDefinition->AmmoReserve);
		}
		WeaponComponent->AddWeapon(Weapon);
	}
}

bool AOutbreakEnemyCharacter::IsDead() const
{
	return HealthComponent->GetHealthState() == EOutbreakHealthState::Dead;
}

void AOutbreakEnemyCharacter::SetEnemyState(EOutbreakEnemyState NewState)
{
	if (!HasAuthority() || EnemyState == NewState)
	{
		return;
	}
	const EOutbreakEnemyState OldState = EnemyState;
	EnemyState = NewState;
	OnEnemyStateChanged(NewState, OldState);
}

void AOutbreakEnemyCharacter::OnRep_EnemyState(EOutbreakEnemyState OldState)
{
	OnEnemyStateChanged(EnemyState, OldState);
}

void AOutbreakEnemyCharacter::HandleDowned(EOutbreakHealthState NewState)
{
	Super::HandleDowned(NewState);

	// The body no longer blocks movement or shots.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (HasAuthority() && EnemyDefinition && EnemyDefinition->CorpseLifetime > 0.0f)
	{
		SetLifeSpan(EnemyDefinition->CorpseLifetime);
	}
}
