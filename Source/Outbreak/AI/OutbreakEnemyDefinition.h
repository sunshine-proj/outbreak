#pragma once

#include "CoreMinimal.h"
#include "Characters/OutbreakMovementSettings.h"
#include "Combat/OutbreakHealthComponent.h"
#include "AI/OutbreakEnemyTactics.h"
#include "AI/OutbreakEnemyTypes.h"
#include "Engine/DataAsset.h"
#include "Stealth/OutbreakVisibility.h"

#include "OutbreakEnemyDefinition.generated.h"

class UBehaviorTree;
class USoundBase;
class UOutbreakWeaponDefinition;

/**
 * An enemy archetype (alien infantry, fast melee attacker, heavy, ...): health, movement, weapon,
 * perception, detection, behaviour, combat range, tactics and targeting. Enemies are tuned here, not
 * in AI code (Phase 5 and Phase 9 acceptance).
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

	// --- Audio (Phase 13) ---

	/**
	 * Bark played where the enemy stands when it enters a state (a growl when Suspicious, a shriek
	 * at Combat). Cosmetic: what other enemies hear is the separate gameplay noise.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TMap<EOutbreakEnemyState, TObjectPtr<USoundBase>> StateSounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> DeathSound;

	/** Seconds between barks, so state flicker does not spam sounds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", Units = "s"))
	float BarkCooldown = 3.0f;

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

	/**
	 * Engine hearing range for a noise of loudness 1 (an upper bound; each noise also has its own
	 * audible range from Project Settings → Outbreak Stealth). 0 = deaf.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0", Units = "cm"))
	float HearingRange = 6000.0f;

	/** Scales how alarming heard noises are (enemy-specific hearing sensitivity). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0"))
	float HearingSensitivity = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0", Units = "s"))
	float HearingMaxAge = 3.0f;

	// --- Behaviour ---

	/** Behavior Tree executing the state (Docs/Phase5-EditorSetup.md). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Behavior")
	TObjectPtr<UBehaviorTree> BehaviorTree;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Behavior", meta = (ClampMin = "0.0", Units = "s"))
	float PatrolWaitTime = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Behavior", meta = (ClampMin = "10.0", Units = "cm"))
	float PatrolAcceptanceRadius = 100.0f;

	// --- Detection (Phase 8) ---

	/** How visibility (light, exposure, movement, distance, field of view, stance) is weighed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Detection")
	FOutbreakVisibilityWeights VisibilityWeights;

	/** Detection gained per second while seeing a fully visible target. 1 = detected after one second. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Detection", meta = (ClampMin = "0.0"))
	float SightDetectionRate = 1.5f;

	/** Detection lost per second while a target is not seen. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Detection", meta = (ClampMin = "0.0"))
	float DetectionDecayRate = 0.3f;

	/** Suspicion lost per second. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Detection", meta = (ClampMin = "0.0"))
	float SuspicionDecayRate = 0.08f;

	/** Suspicion a heard noise adds per unit of perceived loudness. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Detection", meta = (ClampMin = "0.0"))
	float NoiseSuspicionGain = 0.8f;

	/** Suspicion at which the enemy stops and looks. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Detection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SuspiciousThreshold = 0.2f;

	/** Suspicion at which the enemy goes to investigate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Detection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InvestigateThreshold = 0.5f;

	/** Suspicion at which an investigation is urgent (sprint). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Detection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UrgentThreshold = 0.9f;

	/** Seconds a detected target can be out of sight before it counts as lost. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Detection", meta = (ClampMin = "0.0", Units = "s"))
	float LoseTargetTime = 3.0f;

	// --- Searching and alerts ---

	/** Seconds spent searching after losing a target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Search", meta = (ClampMin = "0.0", Units = "s"))
	float SearchDuration = 15.0f;

	/** Search points are picked within this radius of the last known position. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Search", meta = (ClampMin = "0.0", Units = "cm"))
	float SearchRadius = 800.0f;

	/** Seconds spent looking around at each search point. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Search", meta = (ClampMin = "0.0", Units = "s"))
	float SearchPointWait = 1.5f;

	/** Enemies within this distance are alerted when this one detects a hostile. 0 = no propagation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Search", meta = (ClampMin = "0.0", Units = "cm"))
	float AlertPropagationRadius = 2000.0f;

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

	// --- Tactics (Phase 9) ---

	/** Combat style, cover, flanking, coordination, target switching, retreat and leash. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactics")
	FOutbreakEnemyTacticsSettings Tactics;

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
