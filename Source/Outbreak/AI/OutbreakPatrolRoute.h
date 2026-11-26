#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "OutbreakPatrolRoute.generated.h"

/**
 * A patrol route placed in a level: points edited with viewport widgets, relative to the actor.
 * Enemies reference a route; several enemies may share one.
 */
UCLASS()
class OUTBREAK_API AOutbreakPatrolRoute : public AActor
{
	GENERATED_BODY()

public:

	AOutbreakPatrolRoute();

	UFUNCTION(BlueprintPure, Category = "Patrol")
	int32 GetNumPoints() const { return Points.Num(); }

	/** World location of point Index (clamped to the valid range). Actor location if there are no points. */
	UFUNCTION(BlueprintPure, Category = "Patrol")
	FVector GetWorldPoint(int32 Index) const;

	/**
	 * Index after CurrentIndex. Looping routes wrap around; others reverse at the ends (ping-pong).
	 * InOutDirection is +1 or -1 and is updated for ping-pong routes.
	 */
	static int32 GetNextIndex(int32 CurrentIndex, int32 NumPoints, bool bLoop, int32& InOutDirection);

	bool IsLooping() const { return bLoop; }

	/** Replaces the route (procedural routes, tests). Points are relative to the actor. */
	void SetPoints(const TArray<FVector>& InPoints, bool bInLoop) { Points = InPoints; bLoop = bInLoop; }

protected:

	/** Points relative to the actor. */
	UPROPERTY(EditInstanceOnly, Category = "Patrol", meta = (MakeEditWidget = "true"))
	TArray<FVector> Points;

	/** True: last point leads back to the first. False: walk back and forth. */
	UPROPERTY(EditInstanceOnly, Category = "Patrol")
	bool bLoop = true;
};
