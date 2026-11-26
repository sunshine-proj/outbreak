#include "Stealth/OutbreakLightingVolume.h"

#include "Components/BoxComponent.h"

AOutbreakLightingVolume::AOutbreakLightingVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetBoxExtent(FVector(500.0f, 500.0f, 300.0f));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetHiddenInGame(true);
	RootComponent = Bounds;

#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
}

bool AOutbreakLightingVolume::ContainsPoint(const FVector& Point) const
{
	const FVector Local = Bounds->GetComponentTransform().InverseTransformPosition(Point);
	const FVector Extent = Bounds->GetUnscaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}
