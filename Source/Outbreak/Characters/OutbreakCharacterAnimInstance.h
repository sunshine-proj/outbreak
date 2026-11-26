#pragma once

#include "CoreMinimal.h"
#include "AI/OutbreakEnemyTypes.h"
#include "Animation/AnimInstance.h"
#include "Weapons/OutbreakWeaponTypes.h"

#include "OutbreakCharacterAnimInstance.generated.h"

class APawn;

/** Everything an Animation Blueprint needs about a soldier or enemy, gathered once per update. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakCharacterAnimState
{
	GENERATED_BODY()

	/** Horizontal speed, cm/s. */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Speed = 0.0f;

	/** Movement direction relative to facing, -180..180 degrees (0 = forwards), for strafing blend spaces. */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Direction = 0.0f;

	/** View pitch, -90..90 degrees, for aim offsets. */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float AimPitch = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsCrouched = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsReloading = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bHasWeapon = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	EOutbreakWeaponPose WeaponPose = EOutbreakWeaponPose::Unarmed;

	/** Incapacitated, waiting for a revive. */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsDowned = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsDead = false;

	/** Enemies only: their AI state (Idle for soldiers). */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	EOutbreakEnemyState EnemyState = EOutbreakEnemyState::Idle;

	/** Enemies in combat, or soldiers aiming: raise the weapon. */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bCombatReady = false;
};

/**
 * Base class for character Animation Blueprints (Phase 13 animation integration). Reads gameplay
 * state from the owning AOutbreakCharacter each animation update; the Blueprint's graph only maps
 * State to poses, blend spaces and aim offsets. Weapon montages (fire, reload, equip) come from the
 * weapon definition and are played by the character.
 */
UCLASS(Transient, Blueprintable)
class OUTBREAK_API UOutbreakCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** Fills OutState from Pawn (an AOutbreakCharacter; other pawns get movement only). */
	static void GatherState(const APawn* Pawn, FOutbreakCharacterAnimState& OutState);

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FOutbreakCharacterAnimState State;
};
