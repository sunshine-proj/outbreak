#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "OutbreakEnvironmentVolume.generated.h"

class UAudioComponent;
class UBoxComponent;
class UPrimitiveComponent;
class USoundBase;

/** How an environment changes perception (Phase 12 environment-specific perception settings). */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakEnvironmentPerception
{
	GENERATED_BODY()

	/**
	 * Scales enemy sight range for targets or viewers in here: fog, smoke, dense foliage (< 1);
	 * open, clear ground (1). An enemy uses the lower of its own zone and the target's.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float SightRangeMultiplier = 1.0f;

	/**
	 * Scales how loud noises seem to enemies listening from in here: machinery, rain, a waterfall
	 * masking footsteps (< 1); a silent vault (> 1).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float HearingMultiplier = 1.0f;

	/** Scales footstep noise made in here: metal grating, gravel (> 1); grass, carpet, mud (< 1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float FootstepLoudnessMultiplier = 1.0f;
};

/**
 * A zone with its own perception rules and ambience (Phase 12): an indoor facility, a foggy
 * forest, a generator hall. Where zones overlap the highest Priority wins.
 *
 * Environmental audio: AmbientSound (a looping sound) fades in for the local player while they are
 * inside, on top of the level's positional ambient sounds and Audio Volume reverb (engine actors).
 * The ambience is cosmetic; gameplay masking is HearingMultiplier. Uses the engine audio component
 * (Unreal Audio); see O-003.
 */
UCLASS()
class OUTBREAK_API AOutbreakEnvironmentVolume : public AActor
{
	GENERATED_BODY()

public:

	AOutbreakEnvironmentVolume();

	bool ContainsPoint(const FVector& Point) const;

	const FOutbreakEnvironmentPerception& GetPerception() const { return Perception; }
	int32 GetPriority() const { return Priority; }

	void SetPerception(const FOutbreakEnvironmentPerception& NewPerception) { Perception = NewPerception; }
	void SetPriority(int32 NewPriority) { Priority = NewPriority; }

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Environment")
	TObjectPtr<UBoxComponent> Bounds;

	UPROPERTY(VisibleAnywhere, Category = "Environment")
	TObjectPtr<UAudioComponent> Ambience;

	/** Shown in debugging and logs, e.g. "Generator hall". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment")
	FText ZoneName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment")
	FOutbreakEnvironmentPerception Perception;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment")
	int32 Priority = 0;

	/** Looping ambience for the local player inside the zone. Empty = none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> AmbientSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", Units = "s"))
	float AmbienceFadeTime = 1.5f;

private:

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	static bool IsLocalPlayerPawn(const AActor* Actor);
};
