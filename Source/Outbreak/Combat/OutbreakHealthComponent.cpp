#include "Combat/OutbreakHealthComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"
#include "TimerManager.h"
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
	DOREPLIFETIME(UOutbreakHealthComponent, ReviveCount);
	DOREPLIFETIME(UOutbreakHealthComponent, BleedOutServerTime);
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
		StopBleedOut();
		SetHealthState(EOutbreakHealthState::Dead);
		return 0.0f;
	}

	const float OldHealth = Health;
	SetHealth(Health - Damage);

	if (Health <= 0.0f)
	{
		// Out of revives: no second chance.
		const bool bRevivable = bCanBeIncapacitated && (MaxRevives <= 0 || ReviveCount < MaxRevives);
		if (bRevivable)
		{
			// Set the bleed-out time first so state-change listeners can read it.
			StartBleedOut();
		}
		SetHealthState(bRevivable ? EOutbreakHealthState::Incapacitated : EOutbreakHealthState::Dead);
	}

	return OldHealth - Health;
}

void UOutbreakHealthComponent::ResetHealth()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	StopBleedOut();
	ReviveCount = 0;
	SetHealth(MaxHealth);
	SetHealthState(EOutbreakHealthState::Alive);
}

void UOutbreakHealthComponent::ConfigureIncapacitation(bool bInCanBeIncapacitated, float InBleedOutTime, int32 InMaxRevives)
{
	bCanBeIncapacitated = bInCanBeIncapacitated;
	BleedOutTime = FMath::Max(0.0f, InBleedOutTime);
	MaxRevives = FMath::Max(0, InMaxRevives);
}

float UOutbreakHealthComponent::Heal(float Amount)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || HealthState != EOutbreakHealthState::Alive || Amount <= 0.0f)
	{
		return 0.0f;
	}
	const float OldHealth = Health;
	SetHealth(Health + Amount);
	return Health - OldHealth;
}

bool UOutbreakHealthComponent::Revive(float HealthFraction)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || HealthState != EOutbreakHealthState::Incapacitated)
	{
		return false;
	}
	StopBleedOut();
	++ReviveCount;
	SetHealth(FMath::Max(1.0f, MaxHealth * FMath::Clamp(HealthFraction, 0.0f, 1.0f)));
	SetHealthState(EOutbreakHealthState::Alive);
	return true;
}

void UOutbreakHealthComponent::FinishBleedOut()
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || HealthState != EOutbreakHealthState::Incapacitated)
	{
		return;
	}
	StopBleedOut();
	SetHealthState(EOutbreakHealthState::Dead);
}

double UOutbreakHealthComponent::GetServerTimeSeconds() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0;
	}
	const AGameStateBase* GameState = World->GetGameState();
	return GameState ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

float UOutbreakHealthComponent::GetBleedOutTimeRemaining() const
{
	if (HealthState != EOutbreakHealthState::Incapacitated || BleedOutServerTime < 0.0)
	{
		return -1.0f;
	}
	return static_cast<float>(FMath::Max(0.0, BleedOutServerTime - GetServerTimeSeconds()));
}

void UOutbreakHealthComponent::StartBleedOut()
{
	UWorld* World = GetWorld();
	if (BleedOutTime <= 0.0f || !World)
	{
		BleedOutServerTime = -1.0;
		return;
	}
	BleedOutServerTime = GetServerTimeSeconds() + BleedOutTime;
	World->GetTimerManager().SetTimer(BleedOutTimer, this, &ThisClass::FinishBleedOut, BleedOutTime, /*bLoop=*/ false);
}

void UOutbreakHealthComponent::StopBleedOut()
{
	BleedOutServerTime = -1.0;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BleedOutTimer);
	}
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
