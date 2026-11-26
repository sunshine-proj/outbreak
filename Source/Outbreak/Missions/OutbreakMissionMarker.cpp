#include "Missions/OutbreakMissionMarker.h"

#include "Components/SceneComponent.h"
#include "EngineUtils.h"

#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#endif

AOutbreakMissionMarker::AOutbreakMissionMarker()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

#if WITH_EDITORONLY_DATA
	// Objectives look markers up at runtime, so they must always be loaded.
	bIsSpatiallyLoaded = false;

	if (UBillboardComponent* Billboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Billboard")))
	{
		Billboard->SetupAttachment(RootComponent);
	}
#endif
}

AOutbreakMissionMarker* AOutbreakMissionMarker::FindMarker(const UWorld* World, FName MarkerId)
{
	if (!World || MarkerId.IsNone())
	{
		return nullptr;
	}
	for (TActorIterator<AOutbreakMissionMarker> It(World); It; ++It)
	{
		if (IsValid(*It) && It->GetMarkerId() == MarkerId)
		{
			return *It;
		}
	}
	return nullptr;
}
