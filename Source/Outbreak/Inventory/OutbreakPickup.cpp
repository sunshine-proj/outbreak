#include "Inventory/OutbreakPickup.h"

#include "Combat/OutbreakHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Inventory/OutbreakItem.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/OutbreakWeaponDefinition.h"

#define LOCTEXT_NAMESPACE "OutbreakPickup"

namespace
{
	const IOutbreakItem* AsItem(const UObject* Object)
	{
		return Cast<IOutbreakItem>(const_cast<UObject*>(Object));
	}

	UOutbreakInventoryComponent* GetInventory(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<UOutbreakInventoryComponent>() : nullptr;
	}
}

AOutbreakPickup::AOutbreakPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	// The interaction trace (Visibility) must hit it; characters walk over it.
	Mesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AOutbreakPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOutbreakPickup, Item);
	DOREPLIFETIME(AOutbreakPickup, Count);
}

void AOutbreakPickup::SetContents(UObject* NewItem, int32 NewCount)
{
	if (!HasAuthority())
	{
		return;
	}
	Item = AsItem(NewItem) ? NewItem : nullptr;
	Count = FMath::Max(0, NewCount);
	if (!Item || Count <= 0)
	{
		Destroy();
	}
}

bool AOutbreakPickup::CanInteract_Implementation(AActor* InteractionInstigator) const
{
	const UOutbreakInventoryComponent* Inventory = GetInventory(InteractionInstigator);
	const UOutbreakHealthComponent* Health = InteractionInstigator ? InteractionInstigator->FindComponentByClass<UOutbreakHealthComponent>() : nullptr;
	if (!Inventory || !AsItem(Item) || Count <= 0 || (Health && !Health->IsAlive()))
	{
		return false;
	}
	// A weapon can always be taken (swapped); anything else needs room.
	return Cast<UOutbreakWeaponDefinition>(Item) || Inventory->GetAddableCount(Item, Count) > 0;
}

FText AOutbreakPickup::GetInteractionPrompt_Implementation(AActor* InteractionInstigator) const
{
	const IOutbreakItem* ItemInterface = AsItem(Item);
	if (!ItemInterface)
	{
		return FText::GetEmpty();
	}

	const UOutbreakInventoryComponent* Inventory = GetInventory(InteractionInstigator);
	if (const UOutbreakWeaponDefinition* Weapon = Cast<UOutbreakWeaponDefinition>(Item))
	{
		const UOutbreakWeaponDefinition* Carried = Inventory ? Inventory->GetWeaponInSlot(Weapon->EquipmentSlot) : nullptr;
		return Carried
			? FText::Format(LOCTEXT("Swap", "Swap {0} for {1}"), Carried->GetItemDisplayName(), ItemInterface->GetItemDisplayName())
			: FText::Format(LOCTEXT("TakeWeapon", "Take {0}"), ItemInterface->GetItemDisplayName());
	}
	return Count > 1
		? FText::Format(LOCTEXT("TakeMany", "Take {0} x{1}"), ItemInterface->GetItemDisplayName(), Count)
		: FText::Format(LOCTEXT("TakeOne", "Take {0}"), ItemInterface->GetItemDisplayName());
}

void AOutbreakPickup::Interact_Implementation(AActor* InteractionInstigator)
{
	UOutbreakInventoryComponent* Inventory = GetInventory(InteractionInstigator);
	if (!HasAuthority() || !Inventory || !CanInteract_Implementation(InteractionInstigator))
	{
		return;
	}

	if (UOutbreakWeaponDefinition* Weapon = Cast<UOutbreakWeaponDefinition>(Item))
	{
		if (Inventory->GetAddableCount(Weapon, 1) > 0)
		{
			Inventory->AddItem(Weapon, 1);
			SetContents(nullptr, 0);
			return;
		}
		// Slot taken (or too heavy with both): swap, leaving the carried weapon here.
		UOutbreakWeaponDefinition* Replaced = nullptr;
		if (Inventory->SwapWeapon(Weapon, Replaced))
		{
			SetContents(Replaced, Replaced ? 1 : 0);
		}
		return;
	}

	const int32 Taken = Inventory->AddItem(Item, Count);
	SetContents(Item, Count - Taken);
}

#undef LOCTEXT_NAMESPACE
