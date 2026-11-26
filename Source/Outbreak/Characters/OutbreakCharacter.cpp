#include "Characters/OutbreakCharacter.h"

#include "Combat/OutbreakMedicalComponent.h"
#include "Combat/OutbreakMedicalSettings.h"
#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Outbreak.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Stealth/OutbreakNoiseEmitterComponent.h"
#include "Weapons/OutbreakWeaponComponent.h"

#define LOCTEXT_NAMESPACE "OutbreakCharacter"

AOutbreakCharacter::AOutbreakCharacter()
{
	// Movement is ticked by the movement component; the character itself needs no tick.
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	// The engine Pawn profile ignores Visibility, but hitscan shots, the squad command crosshair and
	// line-of-sight checks all trace on Visibility: soldiers and enemies must block it to be hit.
	// (A dedicated weapon trace channel replaces this with the Phase 0 collision channel setup.)
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	HealthComponent = CreateDefaultSubobject<UOutbreakHealthComponent>(TEXT("HealthComponent"));
	WeaponComponent = CreateDefaultSubobject<UOutbreakWeaponComponent>(TEXT("WeaponComponent"));
	SquadMemberComponent = CreateDefaultSubobject<UOutbreakSquadMemberComponent>(TEXT("SquadMemberComponent"));
	NoiseEmitterComponent = CreateDefaultSubobject<UOutbreakNoiseEmitterComponent>(TEXT("NoiseEmitterComponent"));
	InventoryComponent = CreateDefaultSubobject<UOutbreakInventoryComponent>(TEXT("InventoryComponent"));
	MedicalComponent = CreateDefaultSubobject<UOutbreakMedicalComponent>(TEXT("MedicalComponent"));

	// The body turns with the controller's yaw.
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
}

void AOutbreakCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ApplyMovementSettings();

	HealthComponent->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::OnHealthChanged);
	HealthComponent->OnHealthStateChanged.AddUniqueDynamic(this, &ThisClass::OnHealthStateChanged);

	// Squad soldiers go down and can be revived instead of dying outright (Phase 10).
	if (HasAuthority() && Team == EOutbreakTeam::Player)
	{
		const UOutbreakMedicalSettings* Medical = GetDefault<UOutbreakMedicalSettings>();
		if (Medical->bSquadCanBeIncapacitated)
		{
			HealthComponent->ConfigureIncapacitation(true, Medical->BleedOutTime, Medical->MaxRevivesPerSoldier);
		}
	}
}

FGenericTeamId AOutbreakCharacter::GetGenericTeamId() const
{
	return OutbreakTeam::ToGenericTeamId(Team);
}

void AOutbreakCharacter::ApplyMovementSettings()
{
	if (MovementSettings.SprintSpeed < MovementSettings.WalkSpeed)
	{
		UE_LOG(LogOutbreak, Warning, TEXT("%s: SprintSpeed (%.1f) is lower than WalkSpeed (%.1f)."),
			*GetName(), MovementSettings.SprintSpeed, MovementSettings.WalkSpeed);
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = bIsSprinting ? MovementSettings.SprintSpeed : MovementSettings.WalkSpeed;
	Movement->MaxWalkSpeedCrouched = MovementSettings.CrouchSpeed;
	Movement->JumpZVelocity = MovementSettings.JumpZVelocity;
	Movement->AirControl = MovementSettings.AirControl;
	Movement->SetCrouchedHalfHeight(MovementSettings.CrouchedHalfHeight);

	CrouchedEyeHeight = MovementSettings.CrouchedEyeHeight;
	UpdateEyeHeight();
}

void AOutbreakCharacter::SetMovementSettings(const FOutbreakMovementSettings& NewSettings)
{
	MovementSettings = NewSettings;
	ApplyMovementSettings();
}

void AOutbreakCharacter::UpdateEyeHeight()
{
	// ACharacter::RecalculateBaseEyeHeight restores the CDO value on uncrouch, so the eye height is
	// always derived from MovementSettings here instead.
	BaseEyeHeight = bIsCrouched ? MovementSettings.CrouchedEyeHeight : MovementSettings.StandingEyeHeight;
	HandleEyeHeightChanged();
}

bool AOutbreakCharacter::CanSprint() const
{
	return !IsCrouchedOrCrouchRequested() && HealthComponent->IsAlive();
}

bool AOutbreakCharacter::StartSprinting()
{
	if (!CanSprint())
	{
		return false;
	}

	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = MovementSettings.SprintSpeed;
	return true;
}

void AOutbreakCharacter::StopSprinting()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = MovementSettings.WalkSpeed;
}

void AOutbreakCharacter::BeginCrouch()
{
	StopSprinting();
	Crouch();
}

void AOutbreakCharacter::EndCrouch()
{
	UnCrouch();
}

bool AOutbreakCharacter::IsCrouchedOrCrouchRequested() const
{
	return bIsCrouched || GetCharacterMovement()->bWantsToCrouch;
}

void AOutbreakCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	// Covers crouches that were not requested through BeginCrouch (for example from Blueprint).
	StopSprinting();
	UpdateEyeHeight();
}

void AOutbreakCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	UpdateEyeHeight();
}

void AOutbreakCharacter::OnHealthChanged(UOutbreakHealthComponent* InHealthComponent, float NewHealth, float Delta)
{
	if (Delta < 0.0f)
	{
		HandleDamaged(Delta);
	}
}

void AOutbreakCharacter::OnHealthStateChanged(UOutbreakHealthComponent* InHealthComponent,
	EOutbreakHealthState NewState, EOutbreakHealthState OldState)
{
	if (NewState != EOutbreakHealthState::Alive)
	{
		HandleDowned(NewState);
	}
	else if (OldState == EOutbreakHealthState::Incapacitated)
	{
		HandleRevived();
	}
}

void AOutbreakCharacter::HandleRevived()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (HasAuthority())
	{
		WeaponComponent->SetWeaponsEnabled(true);
	}
	OnRevived();
}

// --- Squadmate interaction (revive, heal) --------------------------------------------------------

bool AOutbreakCharacter::CanInteract_Implementation(AActor* InteractionInstigator) const
{
	const AOutbreakCharacter* Helper = Cast<AOutbreakCharacter>(InteractionInstigator);
	if (!Helper || Helper == this || OutbreakTeam::IsHostile(Helper, this))
	{
		return false;
	}
	if (HealthComponent->IsIncapacitated())
	{
		return Helper->GetMedicalComponent() && Helper->GetMedicalComponent()->CanRevive(this);
	}
	return Helper->GetInventoryComponent()
		&& Helper->GetInventoryComponent()->FindUsableItem(EOutbreakItemCategory::Medical, this) != nullptr;
}

FText AOutbreakCharacter::GetInteractionPrompt_Implementation(AActor* InteractionInstigator) const
{
	const FText Name = SquadMemberComponent && !SquadMemberComponent->GetDisplayName().IsEmpty()
		? SquadMemberComponent->GetDisplayName()
		: FText::FromString(GetName());
	if (HealthComponent->IsIncapacitated())
	{
		return FText::Format(LOCTEXT("Revive", "Revive {0}"), Name);
	}
	return FText::Format(LOCTEXT("Heal", "Heal {0}"), Name);
}

void AOutbreakCharacter::Interact_Implementation(AActor* InteractionInstigator)
{
	AOutbreakCharacter* Helper = Cast<AOutbreakCharacter>(InteractionInstigator);
	if (!Helper || !CanInteract_Implementation(InteractionInstigator))
	{
		return;
	}
	if (HealthComponent->IsIncapacitated())
	{
		Helper->GetMedicalComponent()->StartRevive(this);
	}
	else
	{
		Helper->GetInventoryComponent()->UseMedicalOn(this);
	}
}

void AOutbreakCharacter::HandleDowned(EOutbreakHealthState NewState)
{
	// Incapacitated or dead: the character can no longer act until revived (HandleRevived).
	StopSprinting();
	WeaponComponent->StopFire();
	WeaponComponent->SetAiming(false);
	if (HasAuthority())
	{
		WeaponComponent->SetWeaponsEnabled(false);
	}

	GetCharacterMovement()->DisableMovement();

	OnDowned(NewState);
}

#undef LOCTEXT_NAMESPACE
