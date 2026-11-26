#pragma once

#include "CoreMinimal.h"
#include "AI/OutbreakTeammateBehaviorTypes.h"
#include "Engine/DataAsset.h"

#include "OutbreakTeammateAIConfig.generated.h"

class UBehaviorTree;

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

	// --- Following and player distance ---

	/** Distance behind the player of the first follow row. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Following", meta = (ClampMin = "50.0", Units = "cm"))
	float FollowDistance = 300.0f;

	/** Gap between follow positions, so teammates don't converge on one spot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Following", meta = (ClampMin = "50.0", Units = "cm"))
	float FollowSpacing = 150.0f;

	/** Teammates per follow row. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Following", meta = (ClampMin = "1"))
	int32 FollowRowWidth = 3;

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
};
