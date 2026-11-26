#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "OutbreakInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Data asset that groups the player's Enhanced Input assets.
 *
 * Key bindings live in the mapping context asset, so they can be changed without touching code.
 * Create one instance in the editor (see Docs/Phase1-EditorSetup.md) and assign it to the
 * player character Blueprint.
 */
UCLASS(BlueprintType, Const)
class OUTBREAK_API UOutbreakInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Mapping context added for the local player when they possess the character. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mapping")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Priority of DefaultMappingContext. Higher priority contexts win key conflicts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mapping")
	int32 MappingPriority = 0;

	/** Axis2D: X = right, Y = forward. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
	TObjectPtr<UInputAction> MoveAction;

	/** Axis2D: X = yaw, Y = pitch. Inversion and sensitivity belong in the mapping's modifiers. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
	TObjectPtr<UInputAction> LookAction;

	/** Digital. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
	TObjectPtr<UInputAction> JumpAction;

	/** Digital. Toggle or hold, see bToggleCrouch. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
	TObjectPtr<UInputAction> CrouchAction;

	/** Digital, held. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
	TObjectPtr<UInputAction> SprintAction;

	/** Digital. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Interaction")
	TObjectPtr<UInputAction> InteractAction;

	/** Digital, held for automatic fire modes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Weapon")
	TObjectPtr<UInputAction> FireAction;

	/** Digital, held. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Weapon")
	TObjectPtr<UInputAction> AimAction;

	/** Digital. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Weapon")
	TObjectPtr<UInputAction> ReloadAction;

	/** Digital. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Weapon")
	TObjectPtr<UInputAction> NextWeaponAction;

	/** Digital. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Weapon")
	TObjectPtr<UInputAction> PreviousWeaponAction;

	/** Digital. Switches between the active weapon's primary and alternative fire modes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Weapon")
	TObjectPtr<UInputAction> CycleFireModeAction;

	/** Digital. Selects the next compatible ammunition type and reloads with it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Weapon")
	TObjectPtr<UInputAction> CycleAmmoTypeAction;

	/** Digital. Orders the selected teammates to follow the player. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad")
	TObjectPtr<UInputAction> SquadFollowAction;

	/** Digital. Hold current positions. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad")
	TObjectPtr<UInputAction> SquadHoldAction;

	/** Digital. Move to the point under the crosshair. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad")
	TObjectPtr<UInputAction> SquadMoveAction;

	/** Digital. Return to the player. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad")
	TObjectPtr<UInputAction> SquadRegroupAction;

	/** Digital. Attack the hostile under the crosshair. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad")
	TObjectPtr<UInputAction> SquadAttackAction;

	/** Digital. Cancel the selected teammates' orders. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad")
	TObjectPtr<UInputAction> SquadCancelAction;

	/** Digital. Select the whole squad. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad")
	TObjectPtr<UInputAction> SquadSelectAllAction;

	/**
	 * Axis1D. Selects only the teammate whose slot equals the action value. Map one key per slot
	 * with a Scalar modifier (e.g. F1 = 1, F2 = 2, ...), so the number of slots is configuration.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad")
	TObjectPtr<UInputAction> SquadSelectMemberAction;

	/** Axis1D. Like SquadSelectMemberAction, but adds or removes the teammate from the selection. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad")
	TObjectPtr<UInputAction> SquadToggleMemberAction;

	/** Digital. Defend the point under the crosshair (or where teammates stand). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad")
	TObjectPtr<UInputAction> SquadDefendAction;

	/** Digital. Take cover nearby and provide covering fire. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad")
	TObjectPtr<UInputAction> SquadCoverAction;

	/** Digital. Move away from the threat. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad")
	TObjectPtr<UInputAction> SquadRetreatAction;

	/** Digital. Squad stealth mode on / off. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad")
	TObjectPtr<UInputAction> SquadStealthAction;

	/** Digital. Next formation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad")
	TObjectPtr<UInputAction> SquadCycleFormationAction;

	/** Digital. Opens or closes the squad command menu. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad Menu")
	TObjectPtr<UInputAction> SquadMenuAction;

	/** Digital. Menu navigation, mapped only in SquadMenuMappingContext. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad Menu")
	TObjectPtr<UInputAction> SquadMenuNextAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad Menu")
	TObjectPtr<UInputAction> SquadMenuPreviousAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Squad Menu")
	TObjectPtr<UInputAction> SquadMenuConfirmAction;

	// --- Inventory and medical (Phase 10) ---

	/** Use a medical item on yourself. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Inventory")
	TObjectPtr<UInputAction> UseMedicalAction;

	/** Use (throw) the selected utility item. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Inventory")
	TObjectPtr<UInputAction> UseUtilityAction;

	/** Select the next utility item. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Inventory")
	TObjectPtr<UInputAction> CycleUtilityAction;

	/** Open or close the inventory panel. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Inventory")
	TObjectPtr<UInputAction> InventoryAction;

	/**
	 * Added while the squad menu is open, above the default context, so its keys (for example the
	 * mouse wheel and left button) drive the menu instead of weapons.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mapping")
	TObjectPtr<UInputMappingContext> SquadMenuMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mapping")
	int32 SquadMenuMappingPriority = 10;

	/** If true, the crouch input toggles crouching; otherwise crouch is held. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Behaviour")
	bool bToggleCrouch = true;
};
