#include "Characters/OutbreakCharacterAnimInstance.h"

#include "Characters/OutbreakCharacter.h"
#include "Characters/OutbreakEnemyCharacter.h"
#include "Combat/OutbreakHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

void UOutbreakCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	GatherState(TryGetPawnOwner(), State);
}

void UOutbreakCharacterAnimInstance::GatherState(const APawn* Pawn, FOutbreakCharacterAnimState& OutState)
{
	OutState = FOutbreakCharacterAnimState();
	if (!Pawn)
	{
		return;
	}

	const FVector Velocity = Pawn->GetVelocity();
	OutState.Speed = Velocity.Size2D();
	if (OutState.Speed > 1.0f)
	{
		const FRotator Facing = Pawn->GetActorRotation();
		OutState.Direction = FRotator::NormalizeAxis(Velocity.Rotation().Yaw - Facing.Yaw);
	}
	OutState.AimPitch = FRotator::NormalizeAxis(Pawn->GetBaseAimRotation().Pitch);

	const AOutbreakCharacter* Character = Cast<AOutbreakCharacter>(Pawn);
	if (!Character)
	{
		return;
	}

	OutState.bIsCrouched = Character->bIsCrouched;
	OutState.bIsSprinting = Character->IsSprinting();
	if (const UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		OutState.bIsFalling = Movement->IsFalling();
	}

	if (const UOutbreakHealthComponent* Health = Character->GetHealthComponent())
	{
		OutState.bIsDowned = Health->IsIncapacitated();
		OutState.bIsDead = Health->GetHealthState() == EOutbreakHealthState::Dead;
	}

	if (const UOutbreakWeaponComponent* Weapons = Character->GetWeaponComponent())
	{
		FOutbreakWeaponSlot Slot;
		OutState.bHasWeapon = Weapons->GetActiveWeaponSlot(Slot) && Slot.Definition;
		OutState.WeaponPose = OutState.bHasWeapon ? Slot.Definition->WeaponPose : EOutbreakWeaponPose::Unarmed;
		OutState.bIsReloading = Weapons->IsReloading();
		OutState.bIsAiming = Weapons->IsAiming();
	}

	if (const AOutbreakEnemyCharacter* Enemy = Cast<AOutbreakEnemyCharacter>(Character))
	{
		OutState.EnemyState = Enemy->GetEnemyState();
		OutState.bCombatReady = Enemy->IsInCombat();
	}
	else
	{
		OutState.bCombatReady = OutState.bIsAiming;
	}
}
