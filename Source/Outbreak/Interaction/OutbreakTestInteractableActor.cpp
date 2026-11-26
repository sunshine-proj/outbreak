#include "Interaction/OutbreakTestInteractableActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Outbreak.h"
#include "UObject/ConstructorHelpers.h"

AOutbreakTestInteractableActor::AOutbreakTestInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	RootComponent = Mesh;

	// Engine basic shape so the actor is visible without project content. Replaceable in Blueprint.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	ActivatePrompt = NSLOCTEXT("OutbreakInteraction", "TestActivate", "Activate");
	DeactivatePrompt = NSLOCTEXT("OutbreakInteraction", "TestDeactivate", "Deactivate");
}

bool AOutbreakTestInteractableActor::CanInteract_Implementation(AActor* InteractionInstigator) const
{
	return bInteractionEnabled;
}

FText AOutbreakTestInteractableActor::GetInteractionPrompt_Implementation(AActor* InteractionInstigator) const
{
	return bIsActive ? DeactivatePrompt : ActivatePrompt;
}

void AOutbreakTestInteractableActor::Interact_Implementation(AActor* InteractionInstigator)
{
	bIsActive = !bIsActive;
	++InteractionCount;

	UE_LOG(LogOutbreak, Log, TEXT("%s toggled %s by %s."),
		*GetName(), bIsActive ? TEXT("on") : TEXT("off"), *GetNameSafe(InteractionInstigator));

	OnToggled.Broadcast(InteractionInstigator, bIsActive);
}
