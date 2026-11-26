#include "Player/OutbreakPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "Interaction/OutbreakInteractionComponent.h"
#include "Outbreak.h"
#include "Player/OutbreakInputConfig.h"
#include "Player/OutbreakPlayerController.h"
#include "Player/OutbreakSquadCommandComponent.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

AOutbreakPlayerCharacter::AOutbreakPlayerCharacter()
{
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, MovementSettings.StandingEyeHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetFieldOfView(DefaultFieldOfView);

	InteractionComponent = CreateDefaultSubobject<UOutbreakInteractionComponent>(TEXT("InteractionComponent"));

	// The body mesh (none yet) is for other viewers; the owner will see first-person arms.
	GetMesh()->SetOwnerNoSee(true);
}

void AOutbreakPlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	FirstPersonCamera->SetFieldOfView(DefaultFieldOfView);
	WeaponComponent->OnWeaponStateChanged.AddUniqueDynamic(this, &ThisClass::HandleWeaponStateChanged);
}

void AOutbreakPlayerCharacter::HandleEyeHeightChanged()
{
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, BaseEyeHeight));
}

void AOutbreakPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	const APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	if (!InputConfig || !InputConfig->DefaultMappingContext)
	{
		UE_LOG(LogOutbreak, Warning, TEXT("%s: no InputConfig or DefaultMappingContext assigned; player input is disabled."), *GetName());
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		InputSubsystem->AddMappingContext(InputConfig->DefaultMappingContext, InputConfig->MappingPriority);
	}
}

void AOutbreakPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogOutbreak, Error, TEXT("%s: input component is not an EnhancedInputComponent. Check Config/DefaultInput.ini."), *GetName());
		return;
	}

	if (!InputConfig)
	{
		UE_LOG(LogOutbreak, Warning, TEXT("%s: no InputConfig assigned; no input actions are bound."), *GetName());
		return;
	}

	if (InputConfig->MoveAction)
	{
		EnhancedInput->BindAction(InputConfig->MoveAction, ETriggerEvent::Triggered, this, &ThisClass::HandleMove);
	}
	if (InputConfig->LookAction)
	{
		EnhancedInput->BindAction(InputConfig->LookAction, ETriggerEvent::Triggered, this, &ThisClass::HandleLook);
	}
	if (InputConfig->JumpAction)
	{
		EnhancedInput->BindAction(InputConfig->JumpAction, ETriggerEvent::Started, this, &ThisClass::HandleJumpStarted);
		EnhancedInput->BindAction(InputConfig->JumpAction, ETriggerEvent::Completed, this, &ThisClass::HandleJumpCompleted);
	}
	if (InputConfig->CrouchAction)
	{
		EnhancedInput->BindAction(InputConfig->CrouchAction, ETriggerEvent::Started, this, &ThisClass::HandleCrouchStarted);
		EnhancedInput->BindAction(InputConfig->CrouchAction, ETriggerEvent::Completed, this, &ThisClass::HandleCrouchCompleted);
	}
	if (InputConfig->SprintAction)
	{
		EnhancedInput->BindAction(InputConfig->SprintAction, ETriggerEvent::Started, this, &ThisClass::HandleSprintStarted);
		EnhancedInput->BindAction(InputConfig->SprintAction, ETriggerEvent::Completed, this, &ThisClass::HandleSprintCompleted);
	}
	if (InputConfig->InteractAction)
	{
		EnhancedInput->BindAction(InputConfig->InteractAction, ETriggerEvent::Started, this, &ThisClass::HandleInteract);
	}

	if (InputConfig->FireAction)
	{
		EnhancedInput->BindAction(InputConfig->FireAction, ETriggerEvent::Started, this, &ThisClass::HandleFireStarted);
		EnhancedInput->BindAction(InputConfig->FireAction, ETriggerEvent::Completed, this, &ThisClass::HandleFireCompleted);
		EnhancedInput->BindAction(InputConfig->FireAction, ETriggerEvent::Canceled, this, &ThisClass::HandleFireCompleted);
	}
	if (InputConfig->AimAction)
	{
		EnhancedInput->BindAction(InputConfig->AimAction, ETriggerEvent::Started, this, &ThisClass::HandleAimStarted);
		EnhancedInput->BindAction(InputConfig->AimAction, ETriggerEvent::Completed, this, &ThisClass::HandleAimCompleted);
		EnhancedInput->BindAction(InputConfig->AimAction, ETriggerEvent::Canceled, this, &ThisClass::HandleAimCompleted);
	}
	if (InputConfig->ReloadAction)
	{
		EnhancedInput->BindAction(InputConfig->ReloadAction, ETriggerEvent::Started, this, &ThisClass::HandleReload);
	}
	if (InputConfig->NextWeaponAction)
	{
		EnhancedInput->BindAction(InputConfig->NextWeaponAction, ETriggerEvent::Started, this, &ThisClass::HandleNextWeapon);
	}
	if (InputConfig->PreviousWeaponAction)
	{
		EnhancedInput->BindAction(InputConfig->PreviousWeaponAction, ETriggerEvent::Started, this, &ThisClass::HandlePreviousWeapon);
	}
	if (InputConfig->CycleFireModeAction)
	{
		EnhancedInput->BindAction(InputConfig->CycleFireModeAction, ETriggerEvent::Started, this, &ThisClass::HandleCycleFireMode);
	}
	if (InputConfig->CycleAmmoTypeAction)
	{
		EnhancedInput->BindAction(InputConfig->CycleAmmoTypeAction, ETriggerEvent::Started, this, &ThisClass::HandleCycleAmmoType);
	}

	if (InputConfig->SquadFollowAction)
	{
		EnhancedInput->BindAction(InputConfig->SquadFollowAction, ETriggerEvent::Started, this, &ThisClass::HandleSquadFollow);
	}
	if (InputConfig->SquadHoldAction)
	{
		EnhancedInput->BindAction(InputConfig->SquadHoldAction, ETriggerEvent::Started, this, &ThisClass::HandleSquadHold);
	}
	if (InputConfig->SquadMoveAction)
	{
		EnhancedInput->BindAction(InputConfig->SquadMoveAction, ETriggerEvent::Started, this, &ThisClass::HandleSquadMove);
	}
	if (InputConfig->SquadRegroupAction)
	{
		EnhancedInput->BindAction(InputConfig->SquadRegroupAction, ETriggerEvent::Started, this, &ThisClass::HandleSquadRegroup);
	}
	if (InputConfig->SquadAttackAction)
	{
		EnhancedInput->BindAction(InputConfig->SquadAttackAction, ETriggerEvent::Started, this, &ThisClass::HandleSquadAttack);
	}
	if (InputConfig->SquadCancelAction)
	{
		EnhancedInput->BindAction(InputConfig->SquadCancelAction, ETriggerEvent::Started, this, &ThisClass::HandleSquadCancel);
	}
	if (InputConfig->SquadSelectAllAction)
	{
		EnhancedInput->BindAction(InputConfig->SquadSelectAllAction, ETriggerEvent::Started, this, &ThisClass::HandleSquadSelectAll);
	}
	if (InputConfig->SquadSelectMemberAction)
	{
		EnhancedInput->BindAction(InputConfig->SquadSelectMemberAction, ETriggerEvent::Started, this, &ThisClass::HandleSquadSelectMember);
	}
	if (InputConfig->SquadToggleMemberAction)
	{
		EnhancedInput->BindAction(InputConfig->SquadToggleMemberAction, ETriggerEvent::Started, this, &ThisClass::HandleSquadToggleMember);
	}
}

void AOutbreakPlayerCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MoveInput = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), MoveInput.Y);
	AddMovementInput(GetActorRightVector(), MoveInput.X);
}

void AOutbreakPlayerCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void AOutbreakPlayerCharacter::HandleJumpStarted()
{
	Jump();
}

void AOutbreakPlayerCharacter::HandleJumpCompleted()
{
	StopJumping();
}

void AOutbreakPlayerCharacter::HandleCrouchStarted()
{
	if (InputConfig && InputConfig->bToggleCrouch && IsCrouchedOrCrouchRequested())
	{
		EndCrouch();
	}
	else
	{
		BeginCrouch();
	}
}

void AOutbreakPlayerCharacter::HandleCrouchCompleted()
{
	if (InputConfig && !InputConfig->bToggleCrouch)
	{
		EndCrouch();
	}
}

void AOutbreakPlayerCharacter::HandleSprintStarted()
{
	StartSprinting();
}

void AOutbreakPlayerCharacter::HandleSprintCompleted()
{
	StopSprinting();
}

void AOutbreakPlayerCharacter::HandleInteract()
{
	InteractionComponent->TryInteract();
}

void AOutbreakPlayerCharacter::HandleFireStarted()
{
	WeaponComponent->StartFire();
}

void AOutbreakPlayerCharacter::HandleFireCompleted()
{
	WeaponComponent->StopFire();
}

void AOutbreakPlayerCharacter::HandleAimStarted()
{
	WeaponComponent->SetAiming(true);
}

void AOutbreakPlayerCharacter::HandleAimCompleted()
{
	WeaponComponent->SetAiming(false);
}

void AOutbreakPlayerCharacter::HandleReload()
{
	WeaponComponent->StartReload();
}

void AOutbreakPlayerCharacter::HandleNextWeapon()
{
	WeaponComponent->EquipNextWeapon();
}

void AOutbreakPlayerCharacter::HandlePreviousWeapon()
{
	WeaponComponent->EquipPreviousWeapon();
}

void AOutbreakPlayerCharacter::HandleCycleFireMode()
{
	WeaponComponent->CycleFireMode();
}

void AOutbreakPlayerCharacter::HandleCycleAmmoType()
{
	WeaponComponent->CycleAmmoType();
}

void AOutbreakPlayerCharacter::HandleDamaged(float Delta)
{
	// Basic hit reaction: a small upward view kick for the local player who was hit.
	if (IsLocallyControlled() && IsPlayerControlled() && Controller)
	{
		Controller->SetControlRotation(Controller->GetControlRotation() + FRotator(HitReactionPitchKick, 0.0f, 0.0f));
	}
}

void AOutbreakPlayerCharacter::HandleDowned(EOutbreakHealthState NewState)
{
	// Mission failure on player death is Phase 6.
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (PlayerController->IsLocalController())
		{
			DisableInput(PlayerController);
		}
	}

	Super::HandleDowned(NewState);
}

void AOutbreakPlayerCharacter::HandleWeaponStateChanged(UOutbreakWeaponComponent* InWeaponComponent)
{
	const UOutbreakWeaponDefinition* Weapon = WeaponComponent->GetActiveWeaponDefinition();
	const bool bAiming = WeaponComponent->IsAiming() && Weapon;
	FirstPersonCamera->SetFieldOfView(bAiming ? Weapon->AimFieldOfView : DefaultFieldOfView);
}

UOutbreakSquadCommandComponent* AOutbreakPlayerCharacter::GetSquadCommands() const
{
	const AOutbreakPlayerController* PlayerController = Cast<AOutbreakPlayerController>(Controller);
	return PlayerController ? PlayerController->GetSquadCommandComponent() : nullptr;
}

void AOutbreakPlayerCharacter::IssueSquadCommand(EOutbreakSquadCommandType CommandType)
{
	if (UOutbreakSquadCommandComponent* SquadCommands = GetSquadCommands())
	{
		SquadCommands->IssueCommand(CommandType);
	}
}

void AOutbreakPlayerCharacter::HandleSquadFollow()
{
	IssueSquadCommand(EOutbreakSquadCommandType::Follow);
}

void AOutbreakPlayerCharacter::HandleSquadHold()
{
	IssueSquadCommand(EOutbreakSquadCommandType::Hold);
}

void AOutbreakPlayerCharacter::HandleSquadMove()
{
	IssueSquadCommand(EOutbreakSquadCommandType::Move);
}

void AOutbreakPlayerCharacter::HandleSquadRegroup()
{
	IssueSquadCommand(EOutbreakSquadCommandType::Regroup);
}

void AOutbreakPlayerCharacter::HandleSquadAttack()
{
	IssueSquadCommand(EOutbreakSquadCommandType::Attack);
}

void AOutbreakPlayerCharacter::HandleSquadCancel()
{
	if (UOutbreakSquadCommandComponent* SquadCommands = GetSquadCommands())
	{
		SquadCommands->CancelCommands();
	}
}

void AOutbreakPlayerCharacter::HandleSquadSelectAll()
{
	if (UOutbreakSquadCommandComponent* SquadCommands = GetSquadCommands())
	{
		SquadCommands->SelectAll();
	}
}

void AOutbreakPlayerCharacter::HandleSquadSelectMember(const FInputActionValue& Value)
{
	if (UOutbreakSquadCommandComponent* SquadCommands = GetSquadCommands())
	{
		SquadCommands->SelectSlot(FMath::RoundToInt(Value.Get<float>()));
	}
}

void AOutbreakPlayerCharacter::HandleSquadToggleMember(const FInputActionValue& Value)
{
	if (UOutbreakSquadCommandComponent* SquadCommands = GetSquadCommands())
	{
		SquadCommands->ToggleSlot(FMath::RoundToInt(Value.Get<float>()));
	}
}
