#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Characters/OutbreakMovementSettings.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Combat/OutbreakTeam.h"
#include "GenericTeamAgentInterface.h"

#include "OutbreakPlayerCharacter.generated.h"

class UCameraComponent;
class UOutbreakInputConfig;
class UOutbreakInteractionComponent;
class UOutbreakWeaponComponent;
struct FInputActionValue;

/**
 * First-person player character.
 *
 * Owns the first-person camera, movement states (walk, sprint, crouch, jump), input binding and
 * reactions to its own health (hit kick, death). Gameplay systems live in components:
 * interaction, health and weapons (squad membership from Phase 3).
 */
UCLASS(Blueprintable)
class OUTBREAK_API AOutbreakPlayerCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:

	AOutbreakPlayerCharacter();

	//~ Begin AActor / APawn / ACharacter interface
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	//~ End AActor / APawn / ACharacter interface

	//~ Begin IGenericTeamAgentInterface interface
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~ End IGenericTeamAgentInterface interface

	/** Starts sprinting if allowed. Returns true if the character is sprinting afterwards. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool StartSprinting();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StopSprinting();

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsSprinting() const { return bIsSprinting; }

	/** Sprinting is not allowed while crouched or while a crouch is pending. */
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool CanSprint() const;

	/** Stops sprinting and requests a crouch. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void BeginCrouch();

	/** Requests standing up. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void EndCrouch();

	/** True when crouched or when a crouch has been requested but not yet applied by movement. */
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsCrouchedOrCrouchRequested() const;

	/** Pushes MovementSettings to the movement component. Called automatically on initialization. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplyMovementSettings();

	const FOutbreakMovementSettings& GetMovementSettings() const { return MovementSettings; }

	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

	UOutbreakInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

	UOutbreakHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UOutbreakWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UOutbreakInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UOutbreakHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UOutbreakWeaponComponent> WeaponComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team")
	EOutbreakTeam Team = EOutbreakTeam::Player;

	/** Upward view kick when the locally controlled player takes damage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (Units = "deg"))
	float HitReactionPitchKick = 1.5f;

	/** Camera field of view when not aiming. Aiming uses the weapon's AimFieldOfView. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "5.0", ClampMax = "170.0", Units = "deg"))
	float DefaultFieldOfView = 90.0f;

	/** Called on every machine when the character becomes incapacitated or dies. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnDowned(EOutbreakHealthState NewState);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	FOutbreakMovementSettings MovementSettings;

	/** Enhanced Input assets. Without it the character cannot be controlled (a warning is logged). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UOutbreakInputConfig> InputConfig;

private:

	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleJumpStarted();
	void HandleJumpCompleted();
	void HandleCrouchStarted();
	void HandleCrouchCompleted();
	void HandleSprintStarted();
	void HandleSprintCompleted();
	void HandleInteract();
	void HandleFireStarted();
	void HandleFireCompleted();
	void HandleAimStarted();
	void HandleAimCompleted();
	void HandleReload();
	void HandleNextWeapon();
	void HandlePreviousWeapon();
	void HandleCycleFireMode();
	void HandleCycleAmmoType();

	UFUNCTION()
	void HandleHealthChanged(UOutbreakHealthComponent* InHealthComponent, float NewHealth, float Delta);

	UFUNCTION()
	void HandleHealthStateChanged(UOutbreakHealthComponent* InHealthComponent, EOutbreakHealthState NewState, EOutbreakHealthState OldState);

	UFUNCTION()
	void HandleWeaponStateChanged(UOutbreakWeaponComponent* InWeaponComponent);

	/** Sets the camera and eye height for the current crouch state. */
	void UpdateEyeHeight();

	bool bIsSprinting = false;
};
