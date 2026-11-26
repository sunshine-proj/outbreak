#pragma once

#include "CoreMinimal.h"

/**
 * Detection rules (Phase 8). An enemy keeps a detection level 0..1 per target it can see, and an
 * overall suspicion 0..1 fed by sightings and noises:
 * - Detection grows with the target's visibility while seen and decays when not.
 * - A target at detection 1 is detected: the enemy fights it.
 * - Suspicion is at least the highest detection, jumps with noises, and decays slowly.
 * - Suspicion above the suspicious threshold makes the enemy stop and look; above the investigate
 *   threshold it goes to the stimulus.
 */
namespace OutbreakDetection
{
	/** Detection after DeltaSeconds: +Visibility x GainRate per second while seen, -DecayRate when not. */
	OUTBREAK_API float UpdateDetection(float Current, bool bSeen, float Visibility, float GainRate, float DecayRate, float DeltaSeconds);

	/** Suspicion after DeltaSeconds of decay, never below Floor (the highest current detection). */
	OUTBREAK_API float UpdateSuspicion(float Current, float DecayRate, float DeltaSeconds, float Floor);

	/** Suspicion a heard noise adds: perceived loudness x gain x the enemy's hearing sensitivity. */
	OUTBREAK_API float NoiseSuspicion(float PerceivedLoudness, float Gain, float Sensitivity);
}
