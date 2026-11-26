#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "OutbreakMissionMarker.generated.h"

/**
 * A named point in a level that mission objectives refer to by MarkerId (for example the
 * destination of a Reach Location objective). Place it in the level and give it a unique ID.
 */
UCLASS()
class OUTBREAK_API AOutbreakMissionMarker : public AActor
{
	GENERATED_BODY()

public:

	AOutbreakMissionMarker();

	UFUNCTION(BlueprintPure, Category = "Mission")
	FName GetMarkerId() const { return MarkerId; }

	void SetMarkerId(FName NewMarkerId) { MarkerId = NewMarkerId; }

	/** The first marker in World with MarkerId, or null. */
	static AOutbreakMissionMarker* FindMarker(const UWorld* World, FName MarkerId);

protected:

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Mission")
	FName MarkerId;
};
