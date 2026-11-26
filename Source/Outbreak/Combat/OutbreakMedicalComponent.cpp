#include "Combat/OutbreakMedicalComponent.h"

#include "Combat/OutbreakHealthComponent.h"
#include "Combat/OutbreakMedicalSettings.h"
#include "Combat/OutbreakTeam.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Outbreak.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakSquadRoleDefinition.h"
#include "TimerManager.h"

namespace
{
	const UOutbreakHealthComponent* GetHealth(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<UOutbreakHealthComponent>() : nullptr;
	}
}

UOutbreakMedicalComponent::UOutbreakMedicalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UOutbreakMedicalComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UOutbreakMedicalComponent, RevivePatient);
	DOREPLIFETIME(UOutbreakMedicalComponent, ReviveStartServerTime);
	DOREPLIFETIME(UOutbreakMedicalComponent, ReviveDurationSeconds);
}

void UOutbreakMedicalComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReviveTimer);
		World->GetTimerManager().ClearTimer(ConditionTimer);
	}
	Super::EndPlay(EndPlayReason);
}

bool UOutbreakMedicalComponent::HasOwnerAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

double UOutbreakMedicalComponent::GetServerTimeSeconds() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0;
	}
	const AGameStateBase* GameState = World->GetGameState();
	return GameState ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

float UOutbreakMedicalComponent::GetReviveDuration() const
{
	const float BaseDuration = GetDefault<UOutbreakMedicalSettings>()->ReviveDuration;
	const AActor* Owner = GetOwner();
	const UOutbreakSquadMemberComponent* Member = Owner ? Owner->FindComponentByClass<UOutbreakSquadMemberComponent>() : nullptr;
	const UOutbreakSquadRoleDefinition* Role = Member ? Member->GetRole() : nullptr;
	const float Speed = Role ? FMath::Max(0.1f, Role->ReviveSpeedMultiplier) : 1.0f;
	return BaseDuration / Speed;
}

bool UOutbreakMedicalComponent::CanRevive(const AActor* Patient) const
{
	const AActor* Owner = GetOwner();
	const UOutbreakHealthComponent* OwnerHealth = GetHealth(Owner);
	const UOutbreakHealthComponent* PatientHealth = GetHealth(Patient);
	if (!Owner || !Patient || Patient == Owner || IsReviving() || !PatientHealth || !PatientHealth->IsIncapacitated()
		|| (OwnerHealth && !OwnerHealth->IsAlive()) || OutbreakTeam::IsHostile(Owner, Patient))
	{
		return false;
	}
	const UOutbreakMedicalComponent* OtherReviver = FindReviverOf(GetWorld(), Patient);
	if (OtherReviver && OtherReviver != this)
	{
		return false;
	}
	return FVector::Dist(Owner->GetActorLocation(), Patient->GetActorLocation()) <= GetDefault<UOutbreakMedicalSettings>()->ReviveRange;
}

void UOutbreakMedicalComponent::StartRevive(AActor* Patient)
{
	if (HasOwnerAuthority())
	{
		AuthStartRevive(Patient);
	}
	else if (CanRevive(Patient))
	{
		ServerStartRevive(Patient);
	}
}

void UOutbreakMedicalComponent::ServerStartRevive_Implementation(AActor* Patient)
{
	AuthStartRevive(Patient);
}

void UOutbreakMedicalComponent::AuthStartRevive(AActor* Patient)
{
	UWorld* World = GetWorld();
	if (!World || !CanRevive(Patient))
	{
		return;
	}

	RevivePatient = Patient;
	ReviveStartServerTime = GetServerTimeSeconds();
	ReviveDurationSeconds = GetReviveDuration();

	// The reviver stops to work on the patient.
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UPawnMovementComponent* Movement = Character->GetMovementComponent())
		{
			Movement->StopMovementImmediately();
		}
	}

	if (ReviveDurationSeconds <= 0.0f)
	{
		CompleteRevive();
		return;
	}
	World->GetTimerManager().SetTimer(ReviveTimer, this, &ThisClass::CompleteRevive, ReviveDurationSeconds, false);
	World->GetTimerManager().SetTimer(ConditionTimer, this, &ThisClass::CheckReviveConditions, 0.25f, true);
	OnReviveStateChanged.Broadcast(this);
}

void UOutbreakMedicalComponent::CheckReviveConditions()
{
	const AActor* Owner = GetOwner();
	const UOutbreakHealthComponent* OwnerHealth = GetHealth(Owner);
	const UOutbreakHealthComponent* PatientHealth = GetHealth(RevivePatient);
	const bool bStillValid = Owner && RevivePatient && PatientHealth && PatientHealth->IsIncapacitated()
		&& (!OwnerHealth || OwnerHealth->IsAlive())
		&& FVector::Dist(Owner->GetActorLocation(), RevivePatient->GetActorLocation()) <= GetDefault<UOutbreakMedicalSettings>()->ReviveRange;
	if (!bStillValid)
	{
		AuthCancelRevive();
	}
}

void UOutbreakMedicalComponent::CompleteRevive()
{
	if (!HasOwnerAuthority() || !RevivePatient)
	{
		return;
	}
	CheckReviveConditions();
	if (!RevivePatient)
	{
		return;
	}

	UOutbreakHealthComponent* PatientHealth = RevivePatient->FindComponentByClass<UOutbreakHealthComponent>();
	if (PatientHealth && PatientHealth->Revive(GetDefault<UOutbreakMedicalSettings>()->ReviveHealthFraction))
	{
		UE_LOG(LogOutbreak, Log, TEXT("%s revived %s."), *GetNameSafe(GetOwner()), *RevivePatient->GetName());
	}
	AuthCancelRevive();
}

void UOutbreakMedicalComponent::CancelRevive()
{
	if (HasOwnerAuthority())
	{
		AuthCancelRevive();
	}
	else
	{
		ServerCancelRevive();
	}
}

void UOutbreakMedicalComponent::ServerCancelRevive_Implementation()
{
	AuthCancelRevive();
}

void UOutbreakMedicalComponent::AuthCancelRevive()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReviveTimer);
		World->GetTimerManager().ClearTimer(ConditionTimer);
	}
	if (RevivePatient)
	{
		RevivePatient = nullptr;
		OnReviveStateChanged.Broadcast(this);
	}
}

float UOutbreakMedicalComponent::GetReviveProgress() const
{
	if (!RevivePatient || ReviveDurationSeconds <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(static_cast<float>((GetServerTimeSeconds() - ReviveStartServerTime) / ReviveDurationSeconds), 0.0f, 1.0f);
}

UOutbreakMedicalComponent* UOutbreakMedicalComponent::FindReviverOf(const UWorld* World, const AActor* Patient)
{
	if (!World || !Patient)
	{
		return nullptr;
	}
	// Revivers are soldiers (characters); this keeps the search small.
	for (TActorIterator<ACharacter> It(World); It; ++It)
	{
		UOutbreakMedicalComponent* Medical = It->FindComponentByClass<UOutbreakMedicalComponent>();
		if (Medical && Medical->GetRevivePatient() == Patient)
		{
			return Medical;
		}
	}
	return nullptr;
}

void UOutbreakMedicalComponent::OnRep_Revive()
{
	OnReviveStateChanged.Broadcast(this);
}
