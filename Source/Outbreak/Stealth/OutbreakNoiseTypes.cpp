#include "Stealth/OutbreakNoiseTypes.h"

#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "Perception/AISense_Hearing.h"
#include "Stealth/OutbreakStealthSettings.h"

namespace OutbreakNoise
{
	namespace
	{
		TAutoConsoleVariable<int32> CVarDebugNoise(
			TEXT("outbreak.Noise.Debug"), 0,
			TEXT("1 = draw every reported gameplay noise with its audible range."),
			ECVF_Cheat);

		const FName FootstepTag(TEXT("Noise.Footstep"));
		const FName WeaponTag(TEXT("Noise.Weapon"));
		const FName ExplosionTag(TEXT("Noise.Explosion"));
		const FName ImpactTag(TEXT("Noise.Impact"));
		const FName OtherTag(TEXT("Noise.Other"));
	}

	FName GetNoiseTag(EOutbreakNoiseType Type)
	{
		switch (Type)
		{
		case EOutbreakNoiseType::Footstep:  return FootstepTag;
		case EOutbreakNoiseType::Weapon:    return WeaponTag;
		case EOutbreakNoiseType::Explosion: return ExplosionTag;
		case EOutbreakNoiseType::Impact:    return ImpactTag;
		default:                            return OtherTag;
		}
	}

	EOutbreakNoiseType GetNoiseTypeFromTag(FName Tag)
	{
		if (Tag == FootstepTag)  { return EOutbreakNoiseType::Footstep; }
		if (Tag == WeaponTag)    { return EOutbreakNoiseType::Weapon; }
		if (Tag == ExplosionTag) { return EOutbreakNoiseType::Explosion; }
		if (Tag == ImpactTag)    { return EOutbreakNoiseType::Impact; }
		return EOutbreakNoiseType::Other;
	}

	float GetEffectiveRange(const FOutbreakNoiseEvent& Noise)
	{
		const float BaseRange = Noise.RangeOverride > 0.0f
			? Noise.RangeOverride
			: GetDefault<UOutbreakStealthSettings>()->GetBaseNoiseRange(Noise.Type);
		return BaseRange * FMath::Max(0.0f, Noise.Loudness);
	}

	void ReportNoise(UWorld* World, const FOutbreakNoiseEvent& Noise)
	{
		if (!World || World->GetNetMode() == NM_Client || Noise.Loudness <= 0.0f)
		{
			return;
		}

		const float Range = GetEffectiveRange(Noise);
		UAISense_Hearing::ReportNoiseEvent(World, Noise.Location, Noise.Loudness, Noise.Instigator, Range, GetNoiseTag(Noise.Type));

#if ENABLE_DRAW_DEBUG
		if (CVarDebugNoise.GetValueOnGameThread() != 0)
		{
			const FColor Color = Noise.Type == EOutbreakNoiseType::Footstep ? FColor::Cyan
				: Noise.Type == EOutbreakNoiseType::Weapon ? FColor::Red : FColor::Yellow;
			DrawDebugSphere(World, Noise.Location, Range, 24, Color, false, 0.5f);
			DrawDebugString(World, Noise.Location + FVector(0.0f, 0.0f, 50.0f),
				FString::Printf(TEXT("%s %.2f"), *GetNoiseTag(Noise.Type).ToString(), Noise.Loudness), nullptr, Color, 0.5f);
		}
#endif
	}

	float ComputePerceivedLoudness(float Loudness, float Distance, float EffectiveRange, int32 Occluders, float OcclusionAttenuation)
	{
		if (Loudness <= 0.0f || EffectiveRange <= 0.0f || Distance > EffectiveRange)
		{
			return 0.0f;
		}
		const float Falloff = 1.0f - FMath::Max(0.0f, Distance) / EffectiveRange;
		const float Occlusion = FMath::Pow(FMath::Clamp(OcclusionAttenuation, 0.0f, 1.0f), static_cast<float>(FMath::Max(0, Occluders)));
		return Loudness * Falloff * Occlusion;
	}

	int32 CountOccluders(const UWorld* World, const FVector& From, const FVector& To, int32 MaxOccluders,
		const TArray<const AActor*>& IgnoredActors)
	{
		if (!World || MaxOccluders <= 0)
		{
			return 0;
		}

		FCollisionQueryParams Params(SCENE_QUERY_STAT(OutbreakNoiseOcclusion), false);
		for (const AActor* Ignored : IgnoredActors)
		{
			Params.AddIgnoredActor(Ignored);
		}
		// Walls, floors, doors and props muffle sound; characters do not.
		const FCollisionObjectQueryParams ObjectTypes(ECC_TO_BITFIELD(ECC_WorldStatic) | ECC_TO_BITFIELD(ECC_WorldDynamic));

		const FVector Direction = (To - From).GetSafeNormal();
		FVector Start = From;
		int32 Count = 0;
		while (Count < MaxOccluders)
		{
			FHitResult Hit;
			if (!World->LineTraceSingleByObjectType(Hit, Start, To, ObjectTypes, Params))
			{
				break;
			}
			++Count;
			// Continue just past this surface; single-sided collision lets the trace leave the obstacle.
			Start = Hit.ImpactPoint + Direction * 5.0f;
			if (FVector::DotProduct(To - Start, Direction) <= 0.0f)
			{
				break;
			}
		}
		return Count;
	}

	float ComputeMovementLoudness(float Speed, bool bCrouched, bool bSprinting, bool bFalling,
		const FOutbreakMovementNoiseSettings& Settings, float Multiplier)
	{
		if (bFalling || Speed < Settings.MinimumSpeed)
		{
			return 0.0f;
		}
		const float Base = bCrouched ? Settings.CrouchLoudness : (bSprinting ? Settings.SprintLoudness : Settings.WalkLoudness);
		return Base * FMath::Max(0.0f, Multiplier);
	}
}
