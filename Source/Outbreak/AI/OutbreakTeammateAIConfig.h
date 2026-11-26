#pragma once

#include "CoreMinimal.h"
#include "AI/OutbreakTeammateBehaviorTypes.h"
#include "Engine/DataAsset.h"

#include "OutbreakTeammateAIConfig.generated.h"

class UBehaviorTree;
class UEnvQuery;

/**
 * Teammate AI tuning: perception, decision priorities, following, order completion and combat.
 * Assigned on the teammate character (AOutbreakTeammateCharacter::TeammateAIConfig). A teammate
 * without one uses this class's defaults, but then has no Behavior Tree and stands still.
 */
UCLASS(BlueprintType, Const)
class OUTBREAK_API UOutbreakTeammateAIConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UOutbreakTeammateAIConfig();

	/** PriorityOrder if valid, otherwise the default order (a warning is logged once per asset). */
	TArray<EOutbreakBehaviorLayer> GetEffectivePriorityOrder() const;

	bool Validate(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/** Behavior Tree executing the selected behaviour (Docs/Phase4-EditorSetup.md). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Behavior")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	/** First applicable layer wins. Every layer must appear exactly once. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Behavior")
	TArray<EOutbreakBehaviorLayer> PriorityOrder;

	/** Below this health fraction, with a threat present, the teammate falls back to the player. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SurvivalHealthFraction = 0.3f;

	// --- Perception ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "100.0", Units = "cm"))
	float SightRadius = 3000.0f;

	/** Must be at least SightRadius. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "100.0", Units = "cm"))
	float LoseSightRadius = 3500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "5.0", ClampMax = "180.0", Units = "deg"))
	float PeripheralVisionHalfAngle = 70.0f;

	/** Seconds a lost sighting is remembered. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0", Units = "s"))
	float SightMaxAge = 5.0f;

	// --- Following and player distance (slots come from the squad formation) ---

	/** Start moving when farther than this from the follow position. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Following", meta = (ClampMin = "0.0", Units = "cm"))
	float FollowStartDistance = 250.0f;

	/** Stop moving when closer than this. Must be below FollowStartDistance. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Following", meta = (ClampMin = "0.0", Units = "cm"))
	float FollowStopDistance = 100.0f;

	// --- Orders ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orders", meta = (ClampMin = "10.0", Units = "cm"))
	float MoveArrivalRadius = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orders", meta = (ClampMin = "10.0", Units = "cm"))
	float RegroupArrivalRadius = 200.0f;

	// --- Combat ---

	/** Maximum firing distance. The role's PreferredEngagementRange is used for Attack approach when set. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "100.0", Units = "cm"))
	float MaxEngagementRange = 3000.0f;

	/** Fire only when the view is within this angle of the target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.5", ClampMax = "45.0", Units = "deg"))
	float AimTolerance = 8.0f;

	/** With no target, reload when the magazine is below this fraction. An empty magazine always reloads. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IdleReloadFraction = 0.5f;

	// --- Cover, defence and retreat (Phase 7) ---

	/**
	 * EQS query returning cover from the "Outbreak: Threat" context near the "Outbreak: Cover Anchor"
	 * context (Docs/Phase7-EditorSetup.md). Empty = no cover: Cover orders hold position instead.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UEnvQuery> CoverQuery;

	/** Minimum seconds between cover queries per teammate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cover", meta = (ClampMin = "0.1", Units = "s"))
	float CoverQueryInterval = 1.5f;

	/** Seconds a threat's last known position is remembered for cover and retreat. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cover", meta = (ClampMin = "0.0", Units = "s"))
	float ThreatMemory = 10.0f;

	/** A teammate damaged within this many seconds counts as under fire and seeks cover. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cover", meta = (ClampMin = "0.0", Units = "s"))
	float UnderFireMemory = 3.0f;

	/** Defenders keep within this distance of the defended position (cover included). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cover", meta = (ClampMin = "100.0", Units = "cm"))
	float DefendRadius = 800.0f;

	/** Defenders spread around the defended position with this spacing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cover", meta = (ClampMin = "50.0", Units = "cm"))
	float DefendSpacing = 250.0f;

	// --- Squad stealth (Phase 8) ---

	/** Footstep loudness multiplier in squad stealth mode (on top of crouching). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stealth", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StealthNoiseMultiplier = 0.5f;

	/** In stealth mode, freeze when any enemy's detection of this teammate reaches this level. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stealth", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StealthHideThreshold = 0.25f;

	/** How far a retreat moves away from the threat. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cover", meta = (ClampMin = "100.0", Units = "cm"))
	float RetreatDistance = 1500.0f;

	// --- Medical (Phase 10) ---

	/** Uses a medical item on itself at or below this health fraction, when not under fire. 0 = never. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Medical", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SelfHealHealthFraction = 0.35f;

	/** Heals squadmates below this health fraction (with a medical item, without an order). 0 = never. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Medical", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HealOthersHealthFraction = 0.5f;

	/** Squadmates farther than this are left to others. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Medical", meta = (ClampMin = "0.0", Units = "cm"))
	float MedicalAssistRadius = 3000.0f;
};
