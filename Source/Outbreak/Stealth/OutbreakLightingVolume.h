#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "OutbreakLightingVolume.generated.h"

class UBoxComponent;

/**
 * Designer override of the gameplay light level inside a box: a dark corridor, a floodlit yard.
 * Inside the volume the light level is LightLevel, whatever the lights in the level compute. The
 * highest Priority wins where volumes overlap. Has no effect on rendering.
 */
UCLASS()
class OUTBREAK_API AOutbreakLightingVolume : public AActor
{
	GENERATED_BODY()

public:

	AOutbreakLightingVolume();

	bool ContainsPoint(const FVector& Point) const;

	float GetLightLevel() const { return LightLevel; }
	int32 GetPriority() const { return Priority; }

	void SetLightLevel(float NewLightLevel) { LightLevel = FMath::Clamp(NewLightLevel, 0.0f, 1.0f); }

protected:

	UPROPERTY(VisibleAnywhere, Category = "Lighting")
	TObjectPtr<UBoxComponent> Bounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LightLevel = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting")
	int32 Priority = 0;
};
