#include "AI/OutbreakPatrolRoute.h"

#include "Components/SceneComponent.h"

AOutbreakPatrolRoute::AOutbreakPatrolRoute()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
}

FVector AOutbreakPatrolRoute::GetWorldPoint(int32 Index) const
{
	if (Points.IsEmpty())
	{
		return GetActorLocation();
	}
	return GetActorTransform().TransformPosition(Points[FMath::Clamp(Index, 0, Points.Num() - 1)]);
}

int32 AOutbreakPatrolRoute::GetNextIndex(int32 CurrentIndex, int32 NumPoints, bool bLoop, int32& InOutDirection)
{
	if (NumPoints <= 1)
	{
		return 0;
	}
	if (bLoop)
	{
		return (CurrentIndex + 1) % NumPoints;
	}

	InOutDirection = InOutDirection >= 0 ? 1 : -1;
	int32 Next = CurrentIndex + InOutDirection;
	if (Next < 0 || Next >= NumPoints)
	{
		InOutDirection = -InOutDirection;
		Next = CurrentIndex + InOutDirection;
	}
	return FMath::Clamp(Next, 0, NumPoints - 1);
}
