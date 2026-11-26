#include "Combat/OutbreakHealthComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "Net/UnrealNetwork.h"
#include "Outbreak.h"

UOutbreakHealthComponent::UOutbreakHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	SetIsReplicatedByDefault(true);
}

void UOutbreakHealthComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// MaxHealth may have been changed on a Blueprint default; start full.
	Health = MaxHealth;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddUniqueDynamic(this, &ThisClass::HandleTakeAnyDamage);
	}
}

void UOutbreakHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UOutbreakHealthComponent, Health);
	DOREPLIFETIME(UOutbreakHealthComponent, HealthState);
	DOREPLIFETIME(UOutbreakHealthComponent, MaxHealth);
}

void UOutbreakHealthComponent::SetMaxHealth(float NewMaxHealth, bool bRefillHealth)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || NewMaxHealth < 1.0f)
	{
		return;
	}

	MaxHealth = NewMaxHealth;
	SetHealth(bRefillHealth ? MaxHealth : FMath::Min(Health, MaxHealth));
}

float UOutbreakHealthComponent::CalculateDamage(float RawDamage, const UDamageType* DamageType) const
{
	if (RawDamage <= 0.0f)
	{
		return 0.0f;
	}

	float Multiplier = 1.0f;
	if (DamageType)
	{
		const UClass* DamageClass = DamageType->GetClass();
		for (const FOutbreakDamageMultiplier& Entry : DamageMultipliers)
		{
			if (Entry.DamageType && DamageClass->IsChildOf(Entry.DamageType))
			{
				Multiplier *= FMath::Max(0.0f, Entry.Multiplier);
			}
		}
	}

	return RawDamage * Multiplier;
}

float UOutbreakHealthComponent::ApplyDamage(float RawDamage, const UDamageType* DamageType)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || HealthState == EOutbreakHealthState::Dead)
	{
		return 0.0f;
	}

	const float Damage = CalculateDamage(RawDamage, DamageType);
	if (Damage <= 0.0f)
	{
		return 0.0f;
	}

	if (HealthState == EOutbreakHealthState::Incapacitated)
	{
		// Health is already zero; any further damage finishes the owner.
		SetHealthState(EOutbreakHealthState::Dead);
		return 0.0f;
	}

	const float OldHealth = Health;
	SetHealth(Health - Damage);

	if (Health <= 0.0f)
	{
		SetHealthState(bCanBeIncapacitated ? EOutbreakHealthState::Incapacitated : EOutbreakHealthState::Dead);
	}

	return OldHealth - Health;
}

void UOutbreakHealthComponent::ResetHealth()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	SetHealth(MaxHealth);
	SetHealthState(EOutbreakHealthState::Alive);
}

void UOutbreakHealthComponent::SetDamageMultiplier(TSubclassOf<UDamageType> DamageType, float Multiplier)
{
	if (!DamageType)
	{
		return;
	}

	for (FOutbreakDamageMultiplier& Entry : DamageMultipliers)
	{
		if (Entry.DamageType == DamageType)
		{
			Entry.Multiplier = Multiplier;
			return;
		}
	}

	FOutbreakDamageMultiplier& NewEntry = DamageMultipliers.AddDefaulted_GetRef();
	NewEntry.DamageType = DamageType;
	NewEntry.Multiplier = Multiplier;
}

void UOutbreakHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	const float Applied = ApplyDamage(Damage, DamageType);

	UE_LOG(LogOutbreak, Verbose, TEXT("%s took %.1f damage (%s) from %s; health %.1f."),
		*GetNameSafe(DamagedActor), Applied, *GetNameSafe(DamageType ? DamageType->GetClass() : nullptr),
		*GetNameSafe(DamageCauser), Health);
}

void UOutbreakHealthComponent::SetHealth(float NewHealth)
{
	const float OldHealth = Health;
	Health = FMath::Clamp(NewHealth, 0.0f, MaxHealth);

	if (!FMath::IsNearlyEqual(OldHealth, Health))
	{
		OnHealthChanged.Broadcast(this, Health, Health - OldHealth);
	}
}

void UOutbreakHealthComponent::SetHealthState(EOutbreakHealthState NewState)
{
	const EOutbreakHealthState OldState = HealthState;
	if (OldState == NewState)
	{
		return;
	}

	HealthState = NewState;
	UE_LOG(LogOutbreak, Log, TEXT("%s health state: %s -> %s."), *GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(OldState), *UEnum::GetValueAsString(NewState));

	OnHealthStateChanged.Broadcast(this, NewState, OldState);
}

void UOutbreakHealthComponent::OnRep_Health(float OldHealth)
{
	OnHealthChanged.Broadcast(this, Health, Health - OldHealth);
}

void UOutbreakHealthComponent::OnRep_HealthState(EOutbreakHealthState OldState)
{
	OnHealthStateChanged.Broadcast(this, HealthState, OldState);
}
