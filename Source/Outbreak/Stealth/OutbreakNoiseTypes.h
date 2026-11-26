#pragma once

#include "CoreMinimal.h"

#include "OutbreakNoiseTypes.generated.h"

class AActor;
class UWorld;

UENUM(BlueprintType)
enum class EOutbreakNoiseType : uint8
{
	Footstep,
	Weapon,
	Explosion,
	Impact,
	Other
};

/**
 * A gameplay noise: what AI can hear. Independent of the audio backend (O-003): sounds are
 * played by audio code; noises are reported here (D-015).
 */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakNoiseEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	EOutbreakNoiseType Type = EOutbreakNoiseType::Other;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	FVector Location = FVector::ZeroVector;

	/** 1 = normal for the type. Scales the audible range and how alarming the noise is. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = "0.0"))
	float Loudness = 1.0f;

	/** Replaces the type's base range (before loudness scaling). 0 = use the type's range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = "0.0", Units = "cm"))
	float RangeOverride = 0.0f;

	/** Who made the noise. Listeners use its team to ignore friendly noise. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	TObjectPtr<AActor> Instigator;
};

/** Footstep noise by movement state. Lives on UOutbreakNoiseEmitterComponent. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakMovementNoiseSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noise", meta = (ClampMin = "0.0"))
	float CrouchLoudness = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noise", meta = (ClampMin = "0.0"))
	float WalkLoudness = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noise", meta = (ClampMin = "0.0"))
	float SprintLoudness = 1.0f;

	/** Slower than this is silent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noise", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MinimumSpeed = 30.0f;

	/** Seconds between footstep noises while moving. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noise", meta = (ClampMin = "0.05", Units = "s"))
	float FootstepInterval = 0.4f;
};

namespace OutbreakNoise
{
	/** AI hearing tag for a noise type ("Noise.Weapon", ...). */
	OUTBREAK_API FName GetNoiseTag(EOutbreakNoiseType Type);

	/** Inverse of GetNoiseTag; Other for unknown tags. */
	OUTBREAK_API EOutbreakNoiseType GetNoiseTypeFromTag(FName Tag);

	/** Audible range of Noise: base range (or override) scaled by loudness. */
	OUTBREAK_API float GetEffectiveRange(const FOutbreakNoiseEvent& Noise);

	/**
	 * Reports Noise to AI hearing (authority only). Listeners receive it through
	 * UAIPerceptionComponent and apply propagation with ComputePerceivedLoudness.
	 * Console variable `outbreak.Noise.Debug 1` draws each noise.
	 */
	OUTBREAK_API void ReportNoise(UWorld* World, const FOutbreakNoiseEvent& Noise);

	/**
	 * Propagation: loudness falls off linearly to zero at EffectiveRange and is multiplied by
	 * OcclusionAttenuation for each obstacle in between.
	 */
	OUTBREAK_API float ComputePerceivedLoudness(float Loudness, float Distance, float EffectiveRange,
		int32 Occluders, float OcclusionAttenuation);

	/** Number of world-geometry surfaces (static and dynamic objects, not characters) between From and To, up to MaxOccluders. */
	OUTBREAK_API int32 CountOccluders(const UWorld* World, const FVector& From, const FVector& To,
		int32 MaxOccluders, const TArray<const AActor*>& IgnoredActors);

	/** Footstep loudness for a movement state. 0 when slower than MinimumSpeed or airborne. */
	OUTBREAK_API float ComputeMovementLoudness(float Speed, bool bCrouched, bool bSprinting, bool bFalling,
		const FOutbreakMovementNoiseSettings& Settings, float Multiplier);
}
