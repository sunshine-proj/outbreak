#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Stealth/OutbreakNoiseTypes.h"

#include "OutbreakNoiseEmitterComponent.generated.h"

/**
 * Noise generation for an actor (on every Outbreak soldier and enemy).
 *
 * - Footsteps: while the owner character moves, a footstep noise is reported every
 *   FootstepInterval with a loudness from its movement state (crouch < walk < sprint).
 * - ReportNoise: any other noise the owner makes (Blueprint or code).
 *
 * Runs on the authority on a timer, never per frame.
 */
UCLASS(ClassGroup = (Outbreak), meta = (BlueprintSpawnableComponent))
class OUTBREAK_API UOutbreakNoiseEmitterComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UOutbreakNoiseEmitterComponent();

	/** Reports a noise made by the owner at its location. */
	UFUNCTION(BlueprintCallable, Category = "Noise")
	void ReportNoise(EOutbreakNoiseType Type, float Loudness, float RangeOverride = 0.0f);

	/** Scales footstep loudness (squad stealth mode makes teammates quieter). */
	UFUNCTION(BlueprintCallable, Category = "Noise")
	void SetLoudnessMultiplier(float NewMultiplier) { LoudnessMultiplier = FMath::Max(0.0f, NewMultiplier); }

	UFUNCTION(BlueprintPure, Category = "Noise")
	float GetLoudnessMultiplier() const { return LoudnessMultiplier; }

	/** Footstep loudness for the owner's current movement (0 when silent). */
	UFUNCTION(BlueprintPure, Category = "Noise")
	float GetCurrentMovementLoudness() const;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise")
	FOutbreakMovementNoiseSettings MovementNoise;

private:

	void EmitFootstep();

	float LoudnessMultiplier = 1.0f;
	FTimerHandle FootstepTimer;
};
