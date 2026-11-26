#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/** Level queries shared by objective types. */
namespace OutbreakObjectiveQueries
{
	/** Living pawns hostile to the player team within Radius of Location. */
	OUTBREAK_API int32 CountHostilesNear(const UWorld* World, const FVector& Location, float Radius);

	/** The actor is gone, or its health component is dead. */
	OUTBREAK_API bool IsDestroyed(const AActor* Actor);

	/** Every valid actor in World with Tag. */
	OUTBREAK_API TArray<AActor*> FindActorsWithTag(const UWorld* World, FName Tag);

	/** "1:05" */
	OUTBREAK_API FText FormatTime(float Seconds);
}
