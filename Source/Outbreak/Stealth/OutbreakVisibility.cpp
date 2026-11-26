#include "Stealth/OutbreakVisibility.h"

namespace OutbreakVisibility
{
	float ComputeVisibility(const FOutbreakVisibilityInputs& Inputs, const FOutbreakVisibilityWeights& Weights)
	{
		const float LightRange = FMath::Max(0.01f, Weights.BrightLightLevel - Weights.DarkLightLevel);
		const float LightAlpha = FMath::Clamp((Inputs.LightLevel - Weights.DarkLightLevel) / LightRange, 0.0f, 1.0f);
		float Light = FMath::Lerp(Weights.DarknessFactor, 1.0f, LightAlpha);

		float Movement = FMath::Lerp(Weights.StillFactor, 1.0f,
			FMath::Clamp(Inputs.Speed / FMath::Max(1.0f, Weights.FullMovementSpeed), 0.0f, 1.0f));

		const float DistanceAlpha = Inputs.SightRadius > 0.0f ? FMath::Clamp(Inputs.Distance / Inputs.SightRadius, 0.0f, 1.0f) : 1.0f;
		float Distance = 1.0f - DistanceAlpha * DistanceAlpha;

		// Point blank: darkness and stillness no longer hide anyone.
		if (Inputs.Distance <= Weights.CloseRange)
		{
			Light = FMath::Max(Light, 0.8f);
			Movement = FMath::Max(Movement, 0.8f);
			Distance = 1.0f;
		}

		const float FieldOfView = FMath::Lerp(1.0f, Weights.PeripheralFactor,
			FMath::Clamp(Inputs.AngleFromView / FMath::Max(1.0f, Inputs.PeripheralHalfAngle), 0.0f, 1.0f));
		const float Stance = Inputs.bCrouched ? Weights.CrouchFactor : 1.0f;
		const float Exposure = FMath::Clamp(Inputs.ExposedFraction, 0.0f, 1.0f);

		return FMath::Clamp(Light * Exposure * Movement * Distance * FieldOfView * Stance, 0.0f, 1.0f);
	}

	float ComputeLocalLightContribution(float Distance, float AttenuationRadius, float ConeFactor, bool bShadowed, float Scale)
	{
		if (bShadowed || AttenuationRadius <= 0.0f || Distance >= AttenuationRadius)
		{
			return 0.0f;
		}
		const float Falloff = 1.0f - Distance / AttenuationRadius;
		return Falloff * Falloff * FMath::Clamp(ConeFactor, 0.0f, 1.0f) * Scale;
	}

	float ComputeSpotConeFactor(float AngleDegrees, float InnerConeDegrees, float OuterConeDegrees)
	{
		if (AngleDegrees <= InnerConeDegrees)
		{
			return 1.0f;
		}
		if (AngleDegrees >= OuterConeDegrees || OuterConeDegrees <= InnerConeDegrees)
		{
			return 0.0f;
		}
		return 1.0f - (AngleDegrees - InnerConeDegrees) / (OuterConeDegrees - InnerConeDegrees);
	}
}
