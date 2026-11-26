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

	/** If true, the crouch input toggles crouching; otherwise crouch is held. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Behaviour")
	bool bToggleCrouch = true;
};
