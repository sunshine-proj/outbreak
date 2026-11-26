#pragma once

#include "CoreMinimal.h"
#include "Environment/OutbreakEnvironmentVolume.h"
#include "Subsystems/WorldSubsystem.h"

#include "OutbreakEnvironmentSubsystem.generated.h"

/**
 * Environment-specific perception (Phase 12): which perception rules apply at a location, from
 * the AOutbreakEnvironmentVolume zones in the level. Enemy sight and hearing and footstep noise
 * ask it. The zone list is cached and refreshed every few seconds, like the lighting model's.
 */
UCLASS()
class OUTBREAK_API UOutbreakEnvironmentSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/** The highest-priority zone's rules at Location, or neutral rules outside every zone. */
	FOutbreakEnvironmentPerception GetPerceptionAt(const FVector& Location);

	/** Sight range multiplier between a viewer and a target: the lower of the two zones. */
	float GetSightRangeMultiplier(const FVector& ViewerLocation, const FVector& TargetLocation);

	/** Forces the next query to rebuild the zone list. */
	void InvalidateCache() { LastRefreshTime = -1.0e9; }

private:

	void RefreshCacheIfNeeded();

	TArray<TWeakObjectPtr<AOutbreakEnvironmentVolume>> Volumes;
	double LastRefreshTime = -1.0e9;
};
