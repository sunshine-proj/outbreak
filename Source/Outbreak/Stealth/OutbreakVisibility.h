#pragma once

#include "CoreMinimal.h"

#include "OutbreakVisibility.generated.h"

/**
 * How strongly each factor affects how visible a target is to an enemy. On the enemy definition,
 * so archetypes can differ (keen-eyed scouts, near-blind brutes).
 */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakVisibilityWeights
{
	GENERATED_BODY()

	/** Light level at or below which a target counts as in darkness. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DarkLightLevel = 0.1f;

	/** Light level at or above which a target counts as fully lit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BrightLightLevel = 0.6f;

	/** Visibility factor in full darkness (1 = this enemy ignores lighting). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DarknessFactor = 0.15f;

	/** Visibility factor of a motionless target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StillFactor = 0.5f;

	/** Speed at which movement makes a target fully noticeable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility", meta = (ClampMin = "1.0", Units = "cm/s"))
	float FullMovementSpeed = 450.0f;

	/** Visibility factor at the edge of the field of view (1 at its centre). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PeripheralFactor = 0.35f;

	/** Visibility factor of a crouched target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CrouchFactor = 0.7f;

	/** Within this distance darkness and stillness don't hide a target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility", meta = (ClampMin = "0.0", Units = "cm"))
	float CloseRange = 300.0f;
};

/** Everything known about one target, as seen from one enemy. */
struct FOutbreakVisibilityInputs
{
	/** Gameplay light level at the target, 0..1 (UOutbreakLightingSubsystem). */
	float LightLevel = 1.0f;

	/** Fraction of the target's body points in line of sight, 0..1 (obstruction). */
	float ExposedFraction = 1.0f;

	float Speed = 0.0f;
	bool bCrouched = false;
	float Distance = 0.0f;
	float SightRadius = 2500.0f;

	/** Angle between the enemy's view direction and the target, in degrees. */
	float AngleFromView = 0.0f;
	float PeripheralHalfAngle = 60.0f;
};

namespace OutbreakVisibility
{
	/**
	 * Visibility 0..1: light x exposure x movement x distance x field of view x stance. Drives how
	 * fast an enemy's detection of the target grows.
	 */
	OUTBREAK_API float ComputeVisibility(const FOutbreakVisibilityInputs& Inputs, const FOutbreakVisibilityWeights& Weights);

	/** Light added by a point or spot light: quadratic falloff to its radius, 0 when shadowed. */
	OUTBREAK_API float ComputeLocalLightContribution(float Distance, float AttenuationRadius, float ConeFactor,
		bool bShadowed, float Scale);

	/** Spot cone factor: 1 inside the inner cone, 0 outside the outer cone, linear between. */
	OUTBREAK_API float ComputeSpotConeFactor(float AngleDegrees, float InnerConeDegrees, float OuterConeDegrees);
}
