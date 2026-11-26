#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/OutbreakItem.h"

#include "OutbreakWeaponDefinition.generated.h"

class UNiagaraSystem;
class UOutbreakAmmoTypeDefinition;
class UOutbreakFireBehavior;
class USoundBase;

/**
 * Accuracy as a cone half-angle, in degrees.
 *
 * Each shot adds SpreadPerShot of "bloom", which recovers over time. The effective spread is the
 * base spread (hip or aimed) plus bloom, limited to MaxSpread.
 */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakSpreadSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.0", Units = "deg"))
	float HipSpread = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.0", Units = "deg"))
	float AimSpread = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.0", Units = "deg"))
	float SpreadPerShot = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.0", Units = "deg"))
	float MaxSpread = 8.0f;

	/** Bloom recovered per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.0"))
	float RecoveryPerSecond = 6.0f;

	/** Effective cone half-angle for the given aim state and bloom. */
	float GetSpread(bool bAiming, float Bloom) const;

	/** Bloom left after ElapsedSeconds of recovery. */
	float RecoverBloom(float Bloom, float ElapsedSeconds) const;
};

/** View kick applied to the shooter's control rotation per shot. Positive pitch kicks upward. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakRecoilSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (Units = "deg"))
	float PitchKick = 0.6f;

	/** Yaw kick is random in [-YawKickMax, YawKickMax]. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0.0", Units = "deg"))
	float YawKickMax = 0.25f;

	/** Recoil scale while aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0.0"))
	float AimMultiplier = 0.6f;
};

/**
 * One firing mode of a weapon. FireModes[0] is the primary mode; further entries are the
 * alternative modes the player cycles through.
 */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakFireModeDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Mode")
	FText DisplayName;

	/** How a shot is delivered: hitscan, projectile, or a custom subclass. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Fire Mode")
	TObjectPtr<UOutbreakFireBehavior> FireBehavior;

	/** Damage per projectile or trace, before the ammunition multiplier and target resistances. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Mode", meta = (ClampMin = "0.0"))
	float BaseDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Mode", meta = (ClampMin = "1.0"))
	float RoundsPerMinute = 600.0f;

	/** Keep firing while the trigger is held. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Mode")
	bool bAutomatic = true;

	/** Traces or projectiles per shot (more than one for shotgun-style modes). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Mode", meta = (ClampMin = "1"))
	int32 ProjectilesPerShot = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Mode", meta = (ClampMin = "1"))
	int32 AmmoPerShot = 1;

	/** Effective range: hitscan trace length. Projectiles use their own lifetime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Mode", meta = (ClampMin = "1.0", Units = "cm"))
	float Range = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Mode")
	FOutbreakSpreadSettings Spread;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Mode")
	FOutbreakRecoilSettings Recoil;

	/** Seconds between shots. */
	float GetFireInterval() const { return RoundsPerMinute > 0.0f ? 60.0f / RoundsPerMinute : 0.0f; }
};

/**
 * A weapon's configuration. All gameplay values live here so weapons can be created and tuned
 * without changing code. The asset's primary asset ID serves as the weapon ID.
 */
UCLASS(BlueprintType, Const)
class OUTBREAK_API UOutbreakWeaponDefinition : public UPrimaryDataAsset, public IOutbreakItem
{
	GENERATED_BODY()

public:

	/** Returns false and appends a reason for each invalid setting. */
	bool Validate(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	bool IsAmmoTypeCompatible(const UOutbreakAmmoTypeDefinition* AmmoType) const;

	/** Null if FireModeIndex is out of range. */
	const FOutbreakFireModeDefinition* GetFireMode(int32 FireModeIndex) const;

	//~ Begin IOutbreakItem interface
	virtual FText GetItemDisplayName() const override { return DisplayName; }
	virtual EOutbreakItemCategory GetItemCategory() const override { return EOutbreakItemCategory::Weapon; }
	virtual float GetItemUnitWeight() const override { return Weight; }
	virtual int32 GetItemMaxStack() const override { return 1; }
	//~ End IOutbreakItem interface

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FText DisplayName;

	/** Equipment slot (Phase 10): a soldier carries one primary and one secondary weapon. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	EOutbreakEquipmentSlot EquipmentSlot = EOutbreakEquipmentSlot::Primary;

	/** Carried weight, in kilograms, without ammunition. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "0.0", Units = "kg"))
	float Weight = 3.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1"))
	int32 MagazineCapacity = 30;

	/**
	 * The magazine never empties (claws, natural attacks, Phase 9). The weapon still needs a
	 * compatible ammunition type for its damage type, but no reserve.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	bool bUnlimitedAmmo = false;

	/** 0 = instant reload. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0", Units = "s"))
	float ReloadDuration = 2.0f;

	/** At least one. Index 0 is the primary mode. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TArray<FOutbreakFireModeDefinition> FireModes;

	/** At least one. Index 0 is loaded when the weapon is first given. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TArray<TObjectPtr<UOutbreakAmmoTypeDefinition>> CompatibleAmmoTypes;

	/** Camera field of view while aiming. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aiming", meta = (ClampMin = "5.0", ClampMax = "170.0", Units = "deg"))
	float AimFieldOfView = 65.0f;

	/** Muzzle position relative to the view point, used for effects until weapon meshes exist. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	FVector MuzzleOffset = FVector(60.0f, 15.0f, -15.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UNiagaraSystem> MuzzleFlashEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<USoundBase> FireSound;

	/** Gameplay noise of a shot (1 = a normal gunshot). 0 = silent. Suppressed weapons use less. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise", meta = (ClampMin = "0.0"))
	float FireNoiseLoudness = 1.0f;

	/** Audible range at loudness 1, replacing the project's weapon noise range. 0 = project setting. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise", meta = (ClampMin = "0.0", Units = "cm"))
	float FireNoiseRange = 0.0f;
};
