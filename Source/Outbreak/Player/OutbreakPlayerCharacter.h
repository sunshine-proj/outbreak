#pragma once

#include "CoreMinimal.h"
#include "Characters/OutbreakCharacter.h"
#include "Squad/OutbreakSquadCommandTypes.h"

#include "OutbreakPlayerCharacter.generated.h"

class UCameraComponent;
class UOutbreakInputConfig;
class UOutbreakInteractionComponent;
class UOutbreakWeaponComponent;
struct FInputActionValue;

/**
 * First-person player character.
 *
 * Adds the player-only parts to AOutbreakCharacter: first-person camera, input binding,
 * interaction, aim field of view and the hit view kick. Counts as a squad member through the
 * inherited UOutbreakSquadMemberComponent.
 */
UCLASS(Blueprintable)
class OUTBREAK_API AOutbreakPlayerCharacter : public AOutbreakCharacter
{
	GENERATED_BODY()

public:

	AOutbreakPlayerCharacter();

	//~ Begin AActor / APawn interface
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;
	//~ End AActor / APawn interface

	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

	UOutbreakInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

protected:

	//~ Begin AOutbreakCharacter interface
	virtual void HandleDamaged(float Delta) override;
	virtual void HandleDowned(EOutbreakHealthState NewState) override;
	virtual void HandleEyeHeightChanged() override;
	//~ End AOutbreakCharacter interface

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UOutbreakInteractionComponent> InteractionComponent;

	/** Upward view kick when the locally controlled player takes damage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (Units = "deg"))
	float HitReactionPitchKick = 1.5f;

	/** Camera field of view when not aiming, unless the player's settings set one (Phase 14). Aiming uses the weapon's AimFieldOfView. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "5.0", ClampMax = "170.0", Units = "deg"))
	float DefaultFieldOfView = 90.0f;

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
	void HandleUseMedical();
	void HandleUseUtility();
	void HandleCycleUtility();
	void HandleToggleInventory();
	void HandleSquadFollow();
	void HandleSquadHold();
	void HandleSquadMove();
	void HandleSquadRegroup();
	void HandleSquadAttack();
	void HandleSquadCancel();
	void HandleSquadSelectAll();
	void HandleSquadSelectMember(const FInputActionValue& Value);
	void HandleSquadToggleMember(const FInputActionValue& Value);
	void HandleSquadDefend();
	void HandleSquadCover();
	void HandleSquadRetreat();
	void HandleSquadCycleFormation();
	void HandleSquadStealth();
	void HandleSquadMenu();
	void HandleSquadMenuNext();
	void HandleSquadMenuPrevious();
	void HandleSquadMenuConfirm();

	/** Adds or removes the squad menu mapping context to match the menu state. */
	void UpdateSquadMenuInput();

	/** The squad command component of the controlling player controller, if any. */
	class UOutbreakSquadCommandComponent* GetSquadCommands() const;
	void IssueSquadCommand(EOutbreakSquadCommandType CommandType);

	UFUNCTION()
	void HandleWeaponStateChanged(UOutbreakWeaponComponent* InWeaponComponent);

	/** The player's field-of-view setting, else DefaultFieldOfView. */
	float GetBaseFieldOfView() const;

	/** Player settings were applied (Phase 14): refresh the camera. */
	void HandleGameplaySettingsChanged();
};
