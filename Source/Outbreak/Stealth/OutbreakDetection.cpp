#include "Stealth/OutbreakDetection.h"

namespace OutbreakDetection
{
	float UpdateDetection(float Current, bool bSeen, float Visibility, float GainRate, float DecayRate, float DeltaSeconds)
	{
		const float Delta = bSeen
			? FMath::Clamp(Visibility, 0.0f, 1.0f) * FMath::Max(0.0f, GainRate) * DeltaSeconds
			: -FMath::Max(0.0f, DecayRate) * DeltaSeconds;
		return FMath::Clamp(Current + Delta, 0.0f, 1.0f);
	}

	float UpdateSuspicion(float Current, float DecayRate, float DeltaSeconds, float Floor)
	{
		const float Decayed = Current - FMath::Max(0.0f, DecayRate) * FMath::Max(0.0f, DeltaSeconds);
		return FMath::Clamp(FMath::Max(Decayed, Floor), 0.0f, 1.0f);
	}

	float NoiseSuspicion(float PerceivedLoudness, float Gain, float Sensitivity)
	{
		return FMath::Max(0.0f, PerceivedLoudness * Gain * Sensitivity);
	}
}
