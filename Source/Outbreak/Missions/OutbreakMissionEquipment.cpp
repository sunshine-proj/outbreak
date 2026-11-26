#include "Missions/OutbreakMissionEquipment.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Inventory/OutbreakItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Outbreak.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "OutbreakMissionEquipment"

AOutbreakMissionEquipment::AOutbreakMissionEquipment()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Mesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.2f));
	RootComponent = Mesh;

	// Greybox default: the engine cube, console-sized.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	ActivatePrompt = LOCTEXT("DefaultPrompt", "Activate");
}

void AOutbreakMissionEquipment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOutbreakMissionEquipment, bActivated);
	DOREPLIFETIME(AOutbreakMissionEquipment, bLocked);
}

void AOutbreakMissionEquipment::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (HasAuthority() && !bActivated)
	{
		bLocked = bStartsLocked;
	}
}

void AOutbreakMissionEquipment::SetLocked(bool bNewLocked)
{
	if (HasAuthority())
	{
		bLocked = bNewLocked;
	}
}

void AOutbreakMissionEquipment::SetRequiredItem(UOutbreakItemDefinition* NewRequiredItem, bool bConsume)
{
	if (HasAuthority())
	{
		RequiredItem = NewRequiredItem;
		bConsumeRequiredItem = bConsume;
	}
}

bool AOutbreakMissionEquipment::Activate(AActor* ActivatedBy)
{
	if (!HasAuthority() || bActivated)
	{
		return false;
	}
	bActivated = true;
	UE_LOG(LogOutbreak, Log, TEXT("%s activated by %s."), *GetName(), *GetNameSafe(ActivatedBy));
	OnActivated.Broadcast(this, ActivatedBy);
	OnEquipmentActivated();
	return true;
}

void AOutbreakMissionEquipment::OnRep_Activated()
{
	if (bActivated)
	{
		OnActivated.Broadcast(this, nullptr);
		OnEquipmentActivated();
	}
}

bool AOutbreakMissionEquipment::HasRequiredItem(const AActor* InteractionInstigator) const
{
	if (!RequiredItem)
	{
		return true;
	}
	const UOutbreakInventoryComponent* Inventory = InteractionInstigator
		? InteractionInstigator->FindComponentByClass<UOutbreakInventoryComponent>() : nullptr;
	return Inventory && Inventory->GetItemCount(RequiredItem) > 0;
}

bool AOutbreakMissionEquipment::CanInteract_Implementation(AActor* InteractionInstigator) const
{
	// Missing the item still allows focus, so the prompt can say what is needed.
	return !bActivated && !bLocked;
}

FText AOutbreakMissionEquipment::GetInteractionPrompt_Implementation(AActor* InteractionInstigator) const
{
	if (!HasRequiredItem(InteractionInstigator))
	{
		return FText::Format(LOCTEXT("RequiresItem", "Requires {0}"), RequiredItem->DisplayName);
	}
	return ActivatePrompt;
}

void AOutbreakMissionEquipment::Interact_Implementation(AActor* InteractionInstigator)
{
	if (!HasAuthority() || bActivated || bLocked || !HasRequiredItem(InteractionInstigator))
	{
		return;
	}
	if (RequiredItem && bConsumeRequiredItem)
	{
		if (UOutbreakInventoryComponent* Inventory = InteractionInstigator->FindComponentByClass<UOutbreakInventoryComponent>())
		{
			Inventory->RemoveItem(RequiredItem, 1);
		}
	}
	Activate(InteractionInstigator);
}

#undef LOCTEXT_NAMESPACE
