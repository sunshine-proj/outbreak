#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Stealth/OutbreakNoiseTypes.h"

#include "OutbreakStealthSettings.generated.h"

/**
 * Project-wide stealth tuning: Project Settings → Game → Outbreak Stealth (DefaultGame.ini).
 * Enemy-specific sensitivity lives in UOutbreakEnemyDefinition.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Outbreak Stealth"))
class OUTBREAK_API UOutbreakStealthSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** Audible range of a noise of loudness 1 of Type. A noise's range scales with its loudness. */
	float GetBaseNoiseRange(EOutbreakNoiseType Type) const;

	// --- Noise ---

	UPROPERTY(Config, EditAnywhere, Category = "Noise", meta = (ClampMin = "0.0", Units = "cm"))
	float FootstepNoiseRange = 1500.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Noise", meta = (ClampMin = "0.0", Units = "cm"))
	float WeaponNoiseRange = 4000.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Noise", meta = (ClampMin = "0.0", Units = "cm"))
	float ExplosionNoiseRange = 6000.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Noise", meta = (ClampMin = "0.0", Units = "cm"))
	float ImpactNoiseRange = 800.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Noise", meta = (ClampMin = "0.0", Units = "cm"))
	float OtherNoiseRange = 1500.0f;

	/** Fraction of loudness that passes each wall or obstacle between a noise and a listener. */
	UPROPERTY(Config, EditAnywhere, Category = "Noise", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OcclusionAttenuation = 0.5f;

	/** Obstacles counted at most (beyond this the noise is treated as fully muffled by them). */
	UPROPERTY(Config, EditAnywhere, Category = "Noise", meta = (ClampMin = "0", ClampMax = "10"))
	int32 MaxOccluders = 3;

	// --- Lighting (gameplay light level, 0 = dark, 1 = bright) ---

	/** Light level everywhere not covered by an Outbreak Lighting Volume, before lights are added. */
	UPROPERTY(Config, EditAnywhere, Category = "Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AmbientLightLevel = 0.15f;

	/** Added by an unshadowed directional light (sun or moon). */
	UPROPERTY(Config, EditAnywhere, Category = "Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DirectionalLightContribution = 0.6f;

	/** Added at the centre of a point or spot light, falling off to 0 at its attenuation radius. */
	UPROPERTY(Config, EditAnywhere, Category = "Lighting", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float LocalLightContribution = 1.0f;

	/** Seconds between refreshes of the cached light list. */
	UPROPERTY(Config, EditAnywhere, Category = "Lighting", meta = (ClampMin = "0.1", Units = "s"))
	float LightCacheRefreshInterval = 2.0f;

	/** Length of the shadow trace towards a directional light. */
	UPROPERTY(Config, EditAnywhere, Category = "Lighting", meta = (ClampMin = "100.0", Units = "cm"))
	float DirectionalShadowTraceDistance = 20000.0f;
};
