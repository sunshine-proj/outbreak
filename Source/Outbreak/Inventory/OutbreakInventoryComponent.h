#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/OutbreakItem.h"

#include "OutbreakInventoryComponent.generated.h"

class AOutbreakPickup;
class UOutbreakInventoryComponent;
class UOutbreakItemDefinition;
class UOutbreakWeaponComponent;
class UOutbreakWeaponDefinition;
struct FOutbreakLoadout;

/** A number of one item definition (inventory contents, loadouts). */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakItemAmount
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UOutbreakItemDefinition> Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "0"))
	int32 Count = 1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakInventoryChangedSignature, UOutbreakInventoryComponent*, Inventory);

namespace OutbreakInventory
{
	/**
	 * How many of Requested units fit: limited by the remaining weight (MaxWeight - CurrentWeight,
	 * at UnitWeight each) and by the stack limit (MaxStack - CurrentCount; MaxStack 0 = none).
	 */
	OUTBREAK_API int32 ComputeAddableCount(float UnitWeight, float CurrentWeight, float MaxWeight,
		int32 CurrentCount, int32 MaxStack, int32 Requested);
}

/**
 * Everything a soldier carries, limited by weight (Phase 10, PRD §10):
 * - equipment slots: one Primary and one Secondary weapon (stored in the weapon component);
 * - ammunition (stored in the weapon component's reserve, weighed here);
 * - items: medical, utility and mission items (stored here), with a selected utility item.
 *
 * Server-authoritative: changes happen on the authority, contents replicate, and the owner asks
 * for item use through a server RPC.
 */
UCLASS(ClassGroup = (Outbreak), meta = (BlueprintSpawnableComponent))
class OUTBREAK_API UOutbreakInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UOutbreakInventoryComponent();

	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Capacity ---

	/** Weight of weapons, ammunition and items carried, in kilograms. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetCarriedWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetMaxCarryWeight() const { return MaxCarryWeight; }

	/** Authority (loadouts, roles). Items already carried are kept even if now over the limit. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void SetMaxCarryWeight(float NewMaxCarryWeight);

	/**
	 * How many units of Item (a weapon, ammunition type or item definition) could be added now.
	 * A weapon fits only into a free slot of its kind.
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetAddableCount(const UObject* Item, int32 Requested) const;

	// --- Contents ---

	/** Authority. Adds up to Count units of Item and returns how many were added. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	int32 AddItem(UObject* Item, int32 Count);

	/** Authority. Removes up to Count units of Item and returns how many were removed. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	int32 RemoveItem(UObject* Item, int32 Count);

	/** Units of Item carried (weapons 0 or 1, ammunition counts loaded rounds too). */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(const UObject* Item) const;

	/** The medical, utility and mission items carried. */
	const TArray<FOutbreakItemAmount>& GetItems() const { return Items; }

	/** Authority. Drops up to Count units of Item as a pickup in front of the owner. Returns the pickup. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	AOutbreakPickup* DropItem(UObject* Item, int32 Count);

	/** Removes every item (not weapons or ammunition) before a new loadout. Authority. */
	void ClearItems();

	/**
	 * Authority. Replaces all weapons, ammunition and items with Loadout (equipment configuration).
	 * Loadouts are validated as assets, so they are applied without the weight check; the capacity
	 * is changed when the loadout sets one.
	 */
	void ApplyLoadout(const FOutbreakLoadout& Loadout);

	// --- Equipment slots ---

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UOutbreakWeaponDefinition* GetWeaponInSlot(EOutbreakEquipmentSlot Slot) const;

	/**
	 * Authority. Puts NewWeapon into its slot, removing the weapon there (returned in OutReplaced,
	 * not dropped). Fails if the swap would exceed the weight limit.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool SwapWeapon(UOutbreakWeaponDefinition* NewWeapon, UOutbreakWeaponDefinition*& OutReplaced);

	/** The selected utility item (grenade, flare), or null. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UOutbreakItemDefinition* GetSelectedUtility() const;

	/** Selects the next carried utility item. Owner or authority. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CycleUtility();

	// --- Use ---

	/** Whether Item could be used on Target now (carried, usable, in range, owner alive). */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanUseItem(const UOutbreakItemDefinition* Item, const AActor* Target) const;

	/**
	 * Uses one unit of Item on Target (the owner if null). Owner or authority: a client asks the
	 * server. On the authority returns whether it was used.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseItem(UOutbreakItemDefinition* Item, AActor* Target);

	/** The first carried item of Category usable on Target, or null. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UOutbreakItemDefinition* FindUsableItem(EOutbreakItemCategory Category, const AActor* Target) const;

	/** Uses a medical item on Target (the owner if null). See UseItem. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseMedicalOn(AActor* Target);

	/** Uses the selected utility item. See UseItem. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseSelectedUtility();

	/** Fires on every machine when items, weapons, ammunition or capacity change. */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOutbreakInventoryChangedSignature OnInventoryChanged;

protected:

	/** Carrying capacity in kilograms (PRD equipment capacity attribute). Roles can override it. */
	UPROPERTY(EditDefaultsOnly, Replicated, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "0.0", Units = "kg"))
	float MaxCarryWeight = 25.0f;

	/** Items given on initialization (authority), for example on placed test characters. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TArray<FOutbreakItemAmount> DefaultItems;

	/** Spawned by DropItem. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<AOutbreakPickup> DropPickupClass;

private:

	UOutbreakWeaponComponent* GetWeaponComponent() const;
	bool HasOwnerAuthority() const;
	float GetEffectMultiplier() const;
	bool AuthUseItem(UOutbreakItemDefinition* Item, AActor* Target);
	void Broadcast();

	UFUNCTION(Server, Reliable)
	void ServerUseItem(UOutbreakItemDefinition* Item, AActor* Target);

	UFUNCTION(Server, Reliable)
	void ServerCycleUtility();

	UFUNCTION()
	void HandleWeaponStateChanged(UOutbreakWeaponComponent* WeaponComponent);

	UFUNCTION()
	void OnRep_Items();

	UPROPERTY(ReplicatedUsing = OnRep_Items)
	TArray<FOutbreakItemAmount> Items;

	UPROPERTY(ReplicatedUsing = OnRep_Items)
	int32 SelectedUtilityIndex = 0;
};
