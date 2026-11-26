#include "Missions/OutbreakMissionEquipment.h"

#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Environment/OutbreakSwitchable.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Inventory/OutbreakItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Outbreak.h"
#include "Stealth/OutbreakNoiseTypes.h"
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
	DeactivatePrompt = LOCTEXT("DefaultDeactivatePrompt", "Switch off");
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

void AOutbreakMissionEquipment::BeginPlay()
{
	Super::BeginPlay();
	// Linked lights start in the state that matches the equipment; doors keep their own start state.
	ApplyLinkedActors(nullptr, /*bIncludeSwitchables=*/ false);
}

void AOutbreakMissionEquipment::SetLinkedActors(const TArray<AActor*>& InLinkedActors)
{
	if (!HasAuthority())
	{
		return;
	}
	LinkedActors.Reset();
	for (AActor* Actor : InLinkedActors)
	{
		LinkedActors.Add(Actor);
	}
	ApplyLinkedActors(nullptr, /*bIncludeSwitchables=*/ false);
}

void AOutbreakMissionEquipment::ApplyLinkedActors(AActor* SwitchedBy, bool bIncludeSwitchables)
{
	const bool bLightsOn = bActivated == bLinkedLightsOnWhenActive;
	for (AActor* Actor : LinkedActors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}
		if (Actor->GetClass()->ImplementsInterface(UOutbreakSwitchable::StaticClass()))
		{
			if (bIncludeSwitchables && HasAuthority())
			{
				IOutbreakSwitchable::Execute_SetSwitchedOn(Actor, bActivated, SwitchedBy);
			}
			continue;
		}
		// Lights are not replicated; every machine switches its own copy (the stealth model reads visibility live).
		TArray<ULightComponent*> Lights;
		Actor->GetComponents(Lights);
		for (ULightComponent* Light : Lights)
		{
			Light->SetVisibility(bLightsOn);
		}
	}
}

void AOutbreakMissionEquipment::ReportUseNoise(AActor* SwitchedBy) const
{
	if (UseNoiseLoudness > 0.0f)
	{
		FOutbreakNoiseEvent Noise;
		Noise.Type = EOutbreakNoiseType::Other;
		Noise.Location = GetActorLocation();
		Noise.Loudness = UseNoiseLoudness;
		Noise.Instigator = SwitchedBy;
		OutbreakNoise::ReportNoise(GetWorld(), Noise);
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
	ApplyLinkedActors(ActivatedBy, /*bIncludeSwitchables=*/ true);
	ReportUseNoise(ActivatedBy);
	OnActivated.Broadcast(this, ActivatedBy);
	OnEquipmentActivated();
	return true;
}

bool AOutbreakMissionEquipment::Deactivate(AActor* DeactivatedBy)
{
	if (!HasAuthority() || !bActivated)
	{
		return false;
	}
	bActivated = false;
	UE_LOG(LogOutbreak, Log, TEXT("%s switched off by %s."), *GetName(), *GetNameSafe(DeactivatedBy));
	ApplyLinkedActors(DeactivatedBy, /*bIncludeSwitchables=*/ true);
	ReportUseNoise(DeactivatedBy);
	OnDeactivated.Broadcast(this, DeactivatedBy);
	return true;
}

void AOutbreakMissionEquipment::OnRep_Activated()
{
	ApplyLinkedActors(nullptr, /*bIncludeSwitchables=*/ false);
	if (bActivated)
	{
		OnActivated.Broadcast(this, nullptr);
		OnEquipmentActivated();
	}
	else
	{
		OnDeactivated.Broadcast(this, nullptr);
	}
}

bool AOutbreakMissionEquipment::HasRequiredItem(const AActor* InteractionInstigator) const
{
	if (!RequiredItem || bRequiredItemUsed)
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
	return !bLocked && (!bActivated || bCanToggle);
}

FText AOutbreakMissionEquipment::GetInteractionPrompt_Implementation(AActor* InteractionInstigator) const
{
	if (bActivated)
	{
		return DeactivatePrompt;
	}
	if (!HasRequiredItem(InteractionInstigator))
	{
		return FText::Format(LOCTEXT("RequiresItem", "Requires {0}"), RequiredItem->DisplayName);
	}
	return ActivatePrompt;
}

void AOutbreakMissionEquipment::Interact_Implementation(AActor* InteractionInstigator)
{
	if (!HasAuthority() || bLocked)
	{
		return;
	}
	if (bActivated)
	{
		if (bCanToggle)
		{
			Deactivate(InteractionInstigator);
		}
		return;
	}
	if (!HasRequiredItem(InteractionInstigator))
	{
		return;
	}
	if (RequiredItem && !bRequiredItemUsed && bConsumeRequiredItem)
	{
		if (UOutbreakInventoryComponent* Inventory = InteractionInstigator->FindComponentByClass<UOutbreakInventoryComponent>())
		{
			Inventory->RemoveItem(RequiredItem, 1);
		}
	}
	bRequiredItemUsed = RequiredItem != nullptr;
	Activate(InteractionInstigator);
}

#undef LOCTEXT_NAMESPACE
