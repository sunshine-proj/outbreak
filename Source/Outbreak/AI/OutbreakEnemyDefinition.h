#pragma once

#include "CoreMinimal.h"
#include "Characters/OutbreakMovementSettings.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Engine/DataAsset.h"

#include "OutbreakEnemyDefinition.generated.h"

class UBehaviorTree;
class UOutbreakWeaponDefinition;

/**
 * An enemy archetype (alien infantry, ...): health, movement, weapon, perception, behaviour,
 * combat and targeting. Enemies are tuned here, not in AI code (Phase 5 acceptance).
 * Assigned on AOutbreakEnemyCharacter (Blueprint defaults or per placed instance).
 */
UCLASS(BlueprintType, Const)
class OUTBREAK_API UOutbreakEnemyDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	bool Validate(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	FText DisplayName;

	// --- Health ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	/** Resistances and weaknesses by damage type. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	TArray<FOutbreakDamageMultiplier> DamageMultipliers;

	/** Seconds a dead enemy remains before it is removed. 0 = remains. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0.0", Units = "s"))
	float CorpseLifetime = 10.0f;

	// --- Movement ---

	/** Walk speed is used for idle and patrol; sprint speed for alert and combat. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	FOutbreakMovementSettings MovementSettings;

	// --- Weapon ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UOutbreakWeaponDefinition> Weapon;

	/** Reserve ammunition for the weapon's first ammunition type. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
	int32 AmmoReserve = 300;

	// --- Perception ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "100.0", Units = "cm"))
	float SightRadius = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "100.0", Units = "cm"))
	float LoseSightRadius = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "5.0", ClampMax = "180.0", Units = "deg"))
	float PeripheralVisionHalfAngle = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0", Units = "s"))
	float SightMaxAge = 5.0f;

	/** Hearing range for a noise of loudness 1. 0 = deaf. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0", Units = "cm"))
	float HearingRange = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0", Units = "s"))
	float HearingMaxAge = 3.0f;

	// --- Behaviour ---

	/** Behavior Tree executing the state (Docs/Phase5-EditorSetup.md). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Behavior")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	/** Seconds an alert lasts without a new stimulus. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Behavior", meta = (ClampMin = "0.0", Units = "s"))
	float AlertDuration = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Behavior", meta = (ClampMin = "0.0", Units = "s"))
	float PatrolWaitTime = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Behavior", meta = (ClampMin = "10.0", Units = "cm"))
	float PatrolAcceptanceRadius = 100.0f;

	// --- Combat ---

	/** Closer than this, the enemy backs off. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0", Units = "cm"))
	float MinCombatRange = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0", Units = "cm"))
	float PreferredCombatRange = 1200.0f;

	/** Farther than this, the enemy closes in. Also the maximum firing distance. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "100.0", Units = "cm"))
	float MaxCombatRange = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.5", ClampMax = "45.0", Units = "deg"))
	float AimTolerance = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IdleReloadFraction = 0.3f;

	// --- Targeting ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting", meta = (ClampMin = "0.0"))
	float DistancePenaltyPerMeter = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting", meta = (ClampMin = "0.0"))
	float RecentAttackerBonus = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting", meta = (ClampMin = "0.0"))
	float PlayerBonus = 5.0f;

	/** How long an attacker counts as "recent". */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting", meta = (ClampMin = "0.0", Units = "s"))
	float AttackerMemory = 5.0f;
};
