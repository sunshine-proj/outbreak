#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "OutbreakLightingSubsystem.generated.h"

class AOutbreakLightingVolume;
class UDirectionalLightComponent;
class ULocalLightComponent;

/**
 * Gameplay light level at a location, 0 (dark) .. 1 (bright), for stealth (D-015).
 *
 * Computed from the level's lights rather than read back from rendering: ambient level, plus each
 * point and spot light in range that has a clear line to the location, plus unshadowed directional
 * light. An AOutbreakLightingVolume containing the location overrides the result. Light lists are
 * cached and refreshed every LightCacheRefreshInterval seconds.
 */
UCLASS()
class OUTBREAK_API UOutbreakLightingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/** Light level at Location. IgnoredActor (usually the target itself) never casts its own shadow. */
	float GetLightLevelAt(const FVector& Location, const AActor* IgnoredActor = nullptr);

	/** Forces the next query to rebuild the light and volume lists. */
	void InvalidateCache() { LastRefreshTime = -1.0e9; }

private:

	void RefreshCacheIfNeeded();
	bool IsShadowed(const FVector& From, const FVector& To, const AActor* IgnoredActor) const;

	TArray<TWeakObjectPtr<ULocalLightComponent>> LocalLights;
	TArray<TWeakObjectPtr<UDirectionalLightComponent>> DirectionalLights;
	TArray<TWeakObjectPtr<AOutbreakLightingVolume>> Volumes;
	double LastRefreshTime = -1.0e9;
};
