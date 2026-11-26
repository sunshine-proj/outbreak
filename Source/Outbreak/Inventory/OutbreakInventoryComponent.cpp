#include "Inventory/OutbreakInventoryComponent.h"

#include "Combat/OutbreakHealthComponent.h"
#include "Combat/OutbreakMedicalSettings.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Inventory/OutbreakItemAction.h"
#include "Inventory/OutbreakItemDefinition.h"
#include "Inventory/OutbreakLoadout.h"
#include "Inventory/OutbreakPickup.h"
#include "Net/UnrealNetwork.h"
#include "Outbreak.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakSquadRoleDefinition.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

namespace OutbreakInventory
{
	int32 ComputeAddableCount(float UnitWeight, float CurrentWeight, float MaxWeight,
		int32 CurrentCount, int32 MaxStack, int32 Requested)
	{
		if (Requested <= 0)
		{
			return 0;
		}
		const int32 StackRoom = MaxStack > 0 ? FMath::Max(0, MaxStack - CurrentCount) : Requested;
		// A small tolerance so 10 x 0.1 kg fits into exactly 1 kg despite float rounding.
		const int32 WeightRoom = UnitWeight > 0.0f
			? FMath::Max(0, FMath::FloorToInt((MaxWeight - CurrentWeight) / UnitWeight + 1.0e-3f))
			: Requested;
		return FMath::Min3(Requested, StackRoom, WeightRoom);
	}

	/** The item interface of Object, or null. Casts away const only to query the interface. */
	const IOutbreakItem* AsItem(const UObject* Object)
	{
		return Cast<IOutbreakItem>(const_cast<UObject*>(Object));
	}
}

UOutbreakInventoryComponent::UOutbreakInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	SetIsReplicatedByDefault(true);
	DropPickupClass = AOutbreakPickup::StaticClass();
}

void UOutbreakInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UOutbreakInventoryComponent, Items);
	DOREPLIFETIME(UOutbreakInventoryComponent, SelectedUtilityIndex);
	DOREPLIFETIME(UOutbreakInventoryComponent, MaxCarryWeight);
}

void UOutbreakInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (UOutbreakWeaponComponent* Weapons = GetWeaponComponent())
	{
		// Weapons and ammunition live there; their changes change the carried weight.
		Weapons->OnWeaponStateChanged.AddUniqueDynamic(this, &ThisClass::HandleWeaponStateChanged);
	}

	if (HasOwnerAuthority())
	{
		for (const FOutbreakItemAmount& Entry : DefaultItems)
		{
			AddItem(Entry.Item, Entry.Count);
		}
	}
}

UOutbreakWeaponComponent* UOutbreakInventoryComponent::GetWeaponComponent() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UOutbreakWeaponComponent>() : nullptr;
}

bool UOutbreakInventoryComponent::HasOwnerAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

void UOutbreakInventoryComponent::Broadcast()
{
	OnInventoryChanged.Broadcast(this);
}

void UOutbreakInventoryComponent::HandleWeaponStateChanged(UOutbreakWeaponComponent* WeaponComponent)
{
	Broadcast();
}

void UOutbreakInventoryComponent::OnRep_Items()
{
	Broadcast();
}

// --- Capacity ------------------------------------------------------------------------------------

float UOutbreakInventoryComponent::GetCarriedWeight() const
{
	float Weight = 0.0f;
	for (const FOutbreakItemAmount& Entry : Items)
	{
		if (Entry.Item)
		{
			Weight += Entry.Item->Weight * Entry.Count;
		}
	}

	if (const UOutbreakWeaponComponent* Weapons = GetWeaponComponent())
	{
		// Every ammunition type that may be loaded or held in reserve, counted once.
		TSet<const UOutbreakAmmoTypeDefinition*> AmmoTypes;
		for (const UOutbreakWeaponDefinition* Weapon : Weapons->GetWeaponDefinitions())
		{
			Weight += Weapon->Weight;
			for (const UOutbreakAmmoTypeDefinition* AmmoType : Weapon->CompatibleAmmoTypes)
			{
				AmmoTypes.Add(AmmoType);
			}
		}
		for (const FOutbreakAmmoAmount& Reserve : Weapons->GetAmmoReserves())
		{
			AmmoTypes.Add(Reserve.AmmoType.Get());
		}
		for (const UOutbreakAmmoTypeDefinition* AmmoType : AmmoTypes)
		{
			if (AmmoType)
			{
				Weight += AmmoType->WeightPerRound * Weapons->GetTotalAmmo(AmmoType);
			}
		}
	}
	return Weight;
}

void UOutbreakInventoryComponent::SetMaxCarryWeight(float NewMaxCarryWeight)
{
	if (HasOwnerAuthority() && NewMaxCarryWeight >= 0.0f)
	{
		MaxCarryWeight = NewMaxCarryWeight;
		Broadcast();
	}
}

int32 UOutbreakInventoryComponent::GetAddableCount(const UObject* Item, int32 Requested) const
{
	const IOutbreakItem* ItemInterface = OutbreakInventory::AsItem(Item);
	if (!ItemInterface || Requested <= 0)
	{
		return 0;
	}

	const float Carried = GetCarriedWeight();
	switch (ItemInterface->GetItemCategory())
	{
	case EOutbreakItemCategory::Weapon:
	{
		const UOutbreakWeaponDefinition* Weapon = Cast<UOutbreakWeaponDefinition>(Item);
		if (!Weapon || GetWeaponInSlot(Weapon->EquipmentSlot) || !GetWeaponComponent())
		{
			return 0;
		}
		return OutbreakInventory::ComputeAddableCount(Weapon->Weight, Carried, MaxCarryWeight, 0, 1, 1);
	}

	case EOutbreakItemCategory::Ammunition:
		return GetWeaponComponent()
			? OutbreakInventory::ComputeAddableCount(ItemInterface->GetItemUnitWeight(), Carried, MaxCarryWeight, 0, 0, Requested)
			: 0;

	default:
		return OutbreakInventory::ComputeAddableCount(ItemInterface->GetItemUnitWeight(), Carried, MaxCarryWeight,
			GetItemCount(Item), ItemInterface->GetItemMaxStack(), Requested);
	}
}

// --- Contents ------------------------------------------------------------------------------------

int32 UOutbreakInventoryComponent::AddItem(UObject* Item, int32 Count)
{
	if (!HasOwnerAuthority())
	{
		return 0;
	}
	const int32 Addable = GetAddableCount(Item, Count);
	if (Addable <= 0)
	{
		return 0;
	}

	if (UOutbreakWeaponDefinition* Weapon = Cast<UOutbreakWeaponDefinition>(Item))
	{
		return GetWeaponComponent()->AddWeapon(Weapon) != INDEX_NONE ? 1 : 0;
	}
	if (UOutbreakAmmoTypeDefinition* AmmoType = Cast<UOutbreakAmmoTypeDefinition>(Item))
	{
		GetWeaponComponent()->AddAmmo(AmmoType, Addable);
		return Addable;
	}

	UOutbreakItemDefinition* Definition = Cast<UOutbreakItemDefinition>(Item);
	if (!Definition)
	{
		return 0;
	}
	FOutbreakItemAmount* Entry = Items.FindByPredicate([Definition](const FOutbreakItemAmount& Existing) { return Existing.Item == Definition; });
	if (!Entry)
	{
		Entry = &Items.AddDefaulted_GetRef();
		Entry->Item = Definition;
		Entry->Count = 0;
	}
	Entry->Count += Addable;
	Broadcast();
	return Addable;
}

int32 UOutbreakInventoryComponent::RemoveItem(UObject* Item, int32 Count)
{
	if (!HasOwnerAuthority() || !Item || Count <= 0)
	{
		return 0;
	}

	if (UOutbreakWeaponDefinition* Weapon = Cast<UOutbreakWeaponDefinition>(Item))
	{
		UOutbreakWeaponComponent* Weapons = GetWeaponComponent();
		return Weapons && Weapons->RemoveWeapon(Weapon) ? 1 : 0;
	}
	if (UOutbreakAmmoTypeDefinition* AmmoType = Cast<UOutbreakAmmoTypeDefinition>(Item))
	{
		UOutbreakWeaponComponent* Weapons = GetWeaponComponent();
		return Weapons ? Weapons->RemoveAmmo(AmmoType, Count) : 0;
	}

	const int32 Index = Items.IndexOfByPredicate([Item](const FOutbreakItemAmount& Entry) { return Entry.Item.Get() == Item; });
	if (Index == INDEX_NONE)
	{
		return 0;
	}
	const int32 Removed = FMath::Min(Count, Items[Index].Count);
	Items[Index].Count -= Removed;
	if (Items[Index].Count <= 0)
	{
		Items.RemoveAt(Index);
	}
	Broadcast();
	return Removed;
}

int32 UOutbreakInventoryComponent::GetItemCount(const UObject* Item) const
{
	if (!Item)
	{
		return 0;
	}
	const UOutbreakWeaponComponent* Weapons = GetWeaponComponent();
	if (const UOutbreakWeaponDefinition* Weapon = Cast<UOutbreakWeaponDefinition>(Item))
	{
		return Weapons && Weapons->GetWeaponDefinitions().Contains(Weapon) ? 1 : 0;
	}
	if (const UOutbreakAmmoTypeDefinition* AmmoType = Cast<UOutbreakAmmoTypeDefinition>(Item))
	{
		return Weapons ? Weapons->GetTotalAmmo(AmmoType) : 0;
	}
	const FOutbreakItemAmount* Entry = Items.FindByPredicate([Item](const FOutbreakItemAmount& Existing) { return Existing.Item.Get() == Item; });
	return Entry ? Entry->Count : 0;
}

AOutbreakPickup* UOutbreakInventoryComponent::DropItem(UObject* Item, int32 Count)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	const UOutbreakItemDefinition* Definition = Cast<UOutbreakItemDefinition>(Item);
	if (!HasOwnerAuthority() || !World || !Owner || !DropPickupClass || (Definition && !Definition->bCanDrop))
	{
		return nullptr;
	}

	const int32 Removed = RemoveItem(Item, Count);
	if (Removed <= 0)
	{
		return nullptr;
	}

	const FVector Location = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 100.0f;
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AOutbreakPickup* Pickup = World->SpawnActor<AOutbreakPickup>(DropPickupClass, Location, FRotator::ZeroRotator, Parameters);
	if (Pickup)
	{
		Pickup->SetContents(Item, Removed);
	}
	return Pickup;
}

void UOutbreakInventoryComponent::ClearItems()
{
	if (HasOwnerAuthority())
	{
		Items.Reset();
		SelectedUtilityIndex = 0;
		Broadcast();
	}
}

void UOutbreakInventoryComponent::ApplyLoadout(const FOutbreakLoadout& Loadout)
{
	UOutbreakWeaponComponent* Weapons = GetWeaponComponent();
	if (!HasOwnerAuthority() || !Weapons)
	{
		return;
	}

	Weapons->ClearLoadout();
	Items.Reset();
	SelectedUtilityIndex = 0;
	if (Loadout.CarryCapacity > 0.0f)
	{
		MaxCarryWeight = Loadout.CarryCapacity;
	}

	for (const FOutbreakAmmoAmount& Entry : Loadout.Ammo)
	{
		Weapons->AddAmmo(Entry.AmmoType, Entry.Count);
	}
	// Primary first, so it is the weapon in hand.
	for (UOutbreakWeaponDefinition* Weapon : { Loadout.PrimaryWeapon.Get(), Loadout.SecondaryWeapon.Get() })
	{
		if (Weapon)
		{
			Weapons->AddWeapon(Weapon);
		}
	}
	for (const FOutbreakItemAmount& Entry : Loadout.Items)
	{
		if (!Entry.Item || Entry.Count <= 0)
		{
			continue;
		}
		FOutbreakItemAmount* Existing = Items.FindByPredicate([&Entry](const FOutbreakItemAmount& Candidate) { return Candidate.Item == Entry.Item; });
		if (Existing)
		{
			Existing->Count += Entry.Count;
		}
		else
		{
			Items.Add(Entry);
		}
	}
	Broadcast();
}

// --- Equipment slots -----------------------------------------------------------------------------

UOutbreakWeaponDefinition* UOutbreakInventoryComponent::GetWeaponInSlot(EOutbreakEquipmentSlot Slot) const
{
	if (const UOutbreakWeaponComponent* Weapons = GetWeaponComponent())
	{
		for (UOutbreakWeaponDefinition* Weapon : Weapons->GetWeaponDefinitions())
		{
			if (Weapon->EquipmentSlot == Slot)
			{
				return Weapon;
			}
		}
	}
	return nullptr;
}

bool UOutbreakInventoryComponent::SwapWeapon(UOutbreakWeaponDefinition* NewWeapon, UOutbreakWeaponDefinition*& OutReplaced)
{
	OutReplaced = nullptr;
	UOutbreakWeaponComponent* Weapons = GetWeaponComponent();
	if (!HasOwnerAuthority() || !NewWeapon || !Weapons || Weapons->GetWeaponDefinitions().Contains(NewWeapon))
	{
		return false;
	}

	UOutbreakWeaponDefinition* Replaced = GetWeaponInSlot(NewWeapon->EquipmentSlot);
	const float WeightAfter = GetCarriedWeight() - (Replaced ? Replaced->Weight : 0.0f) + NewWeapon->Weight;
	if (WeightAfter > MaxCarryWeight + 1.0e-3f)
	{
		return false;
	}

	if (Replaced)
	{
		Weapons->RemoveWeapon(Replaced);
	}
	if (Weapons->AddWeapon(NewWeapon) == INDEX_NONE)
	{
		// Invalid weapon: put the old one back.
		if (Replaced)
		{
			Weapons->AddWeapon(Replaced);
		}
		return false;
	}
	OutReplaced = Replaced;
	return true;
}

UOutbreakItemDefinition* UOutbreakInventoryComponent::GetSelectedUtility() const
{
	TArray<UOutbreakItemDefinition*> Utilities;
	for (const FOutbreakItemAmount& Entry : Items)
	{
		if (Entry.Item && Entry.Count > 0 && Entry.Item->Category == EOutbreakItemCategory::Utility)
		{
			Utilities.Add(Entry.Item);
		}
	}
	return Utilities.IsEmpty() ? nullptr : Utilities[FMath::Abs(SelectedUtilityIndex) % Utilities.Num()];
}

void UOutbreakInventoryComponent::CycleUtility()
{
	if (!HasOwnerAuthority())
	{
		ServerCycleUtility();
		return;
	}
	++SelectedUtilityIndex;
	Broadcast();
}

void UOutbreakInventoryComponent::ServerCycleUtility_Implementation()
{
	CycleUtility();
}

// --- Use -----------------------------------------------------------------------------------------

float UOutbreakInventoryComponent::GetEffectMultiplier() const
{
	const AActor* Owner = GetOwner();
	const UOutbreakSquadMemberComponent* Member = Owner ? Owner->FindComponentByClass<UOutbreakSquadMemberComponent>() : nullptr;
	const UOutbreakSquadRoleDefinition* Role = Member ? Member->GetRole() : nullptr;
	return Role ? Role->MedicalEffectMultiplier : 1.0f;
}

bool UOutbreakInventoryComponent::CanUseItem(const UOutbreakItemDefinition* Item, const AActor* Target) const
{
	AActor* Owner = GetOwner();
	if (!Owner || !Item || !Item->UseAction || GetItemCount(Item) <= 0)
	{
		return false;
	}
	const UOutbreakHealthComponent* OwnerHealth = Owner->FindComponentByClass<UOutbreakHealthComponent>();
	if (OwnerHealth && !OwnerHealth->IsAlive())
	{
		return false;
	}

	AActor* ResolvedTarget = Target ? const_cast<AActor*>(Target) : Owner;
	if (ResolvedTarget != Owner)
	{
		const float Range = GetDefault<UOutbreakMedicalSettings>()->MedicalUseRange;
		if (!Item->UseAction->CanTargetOthers() || FVector::Dist(Owner->GetActorLocation(), ResolvedTarget->GetActorLocation()) > Range)
		{
			return false;
		}
	}

	FOutbreakItemUseContext Context;
	Context.User = Owner;
	Context.Target = ResolvedTarget;
	Context.EffectMultiplier = Item->Category == EOutbreakItemCategory::Medical ? GetEffectMultiplier() : 1.0f;
	return Item->UseAction->CanUse(Context);
}

bool UOutbreakInventoryComponent::UseItem(UOutbreakItemDefinition* Item, AActor* Target)
{
	if (HasOwnerAuthority())
	{
		return AuthUseItem(Item, Target);
	}
	if (!CanUseItem(Item, Target))
	{
		return false;
	}
	ServerUseItem(Item, Target);
	return true;
}

void UOutbreakInventoryComponent::ServerUseItem_Implementation(UOutbreakItemDefinition* Item, AActor* Target)
{
	AuthUseItem(Item, Target);
}

bool UOutbreakInventoryComponent::AuthUseItem(UOutbreakItemDefinition* Item, AActor* Target)
{
	if (!CanUseItem(Item, Target))
	{
		return false;
	}

	FOutbreakItemUseContext Context;
	Context.User = GetOwner();
	Context.Target = Target ? Target : GetOwner();
	Context.EffectMultiplier = Item->Category == EOutbreakItemCategory::Medical ? GetEffectMultiplier() : 1.0f;

	if (!Item->UseAction->Use(Context))
	{
		return false;
	}
	RemoveItem(Item, 1);
	UE_LOG(LogOutbreak, Log, TEXT("%s used %s on %s."), *GetNameSafe(GetOwner()), *Item->GetName(), *GetNameSafe(Context.Target));
	return true;
}

UOutbreakItemDefinition* UOutbreakInventoryComponent::FindUsableItem(EOutbreakItemCategory Category, const AActor* Target) const
{
	for (const FOutbreakItemAmount& Entry : Items)
	{
		if (Entry.Item && Entry.Item->Category == Category && CanUseItem(Entry.Item, Target))
		{
			return Entry.Item;
		}
	}
	return nullptr;
}

bool UOutbreakInventoryComponent::UseMedicalOn(AActor* Target)
{
	UOutbreakItemDefinition* Item = FindUsableItem(EOutbreakItemCategory::Medical, Target);
	return Item && UseItem(Item, Target);
}

bool UOutbreakInventoryComponent::UseSelectedUtility()
{
	UOutbreakItemDefinition* Item = GetSelectedUtility();
	return Item && UseItem(Item, nullptr);
}
