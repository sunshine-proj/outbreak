#include "Squad/OutbreakSquadSnapshot.h"

#include "Combat/OutbreakHealthComponent.h"
#include "Combat/OutbreakMedicalSettings.h"
#include "GameFramework/Actor.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Inventory/OutbreakItemDefinition.h"
#include "Inventory/OutbreakLoadout.h"
#include "Outbreak.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakSquadSelection.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

namespace OutbreakSquadSnapshot
{
	namespace
	{
		template <typename AssetType>
		AssetType* LoadSaved(const TSoftObjectPtr<AssetType>& Saved, const AActor& Soldier)
		{
			if (Saved.IsNull())
			{
				return nullptr;
			}
			AssetType* Asset = Saved.LoadSynchronous();
			if (!Asset)
			{
				UE_LOG(LogOutbreak, Warning, TEXT("%s: saved asset %s no longer exists; skipped."), *Soldier.GetName(), *Saved.ToString());
			}
			return Asset;
		}
	}

	FOutbreakSoldierSnapshot CaptureSoldier(const AActor& Soldier)
	{
		FOutbreakSoldierSnapshot Snapshot;

		if (const UOutbreakHealthComponent* Health = Soldier.FindComponentByClass<UOutbreakHealthComponent>())
		{
			Snapshot.bDead = Health->GetHealthState() == EOutbreakHealthState::Dead;
			Snapshot.HealthFraction = Health->GetHealthFraction();
			Snapshot.ReviveCount = Health->GetReviveCount();
		}

		if (const UOutbreakWeaponComponent* Weapons = Soldier.FindComponentByClass<UOutbreakWeaponComponent>())
		{
			for (const FOutbreakWeaponSlot& Slot : Weapons->GetWeaponSlots())
			{
				FOutbreakSavedWeapon& Saved = Snapshot.Weapons.AddDefaulted_GetRef();
				Saved.Weapon = Slot.Definition.Get();
				Saved.LoadedAmmoType = Slot.LoadedAmmoType.Get();
				Saved.SelectedAmmoType = Slot.SelectedAmmoType.Get();
				Saved.AmmoInMagazine = Slot.AmmoInMagazine;
				Saved.FireModeIndex = Slot.FireModeIndex;
			}
			Snapshot.ActiveWeaponIndex = Weapons->GetActiveWeaponIndex();
			for (const FOutbreakAmmoAmount& Reserve : Weapons->GetAmmoReserves())
			{
				if (Reserve.AmmoType && Reserve.Count > 0)
				{
					FOutbreakSavedAmmo& Saved = Snapshot.AmmoReserves.AddDefaulted_GetRef();
					Saved.AmmoType = Reserve.AmmoType.Get();
					Saved.Count = Reserve.Count;
				}
			}
		}

		if (const UOutbreakInventoryComponent* Inventory = Soldier.FindComponentByClass<UOutbreakInventoryComponent>())
		{
			for (const FOutbreakItemAmount& Entry : Inventory->GetItems())
			{
				if (Entry.Item && Entry.Count > 0)
				{
					FOutbreakSavedItem& Saved = Snapshot.Items.AddDefaulted_GetRef();
					Saved.Item = Entry.Item.Get();
					Saved.Count = Entry.Count;
				}
			}
		}
		return Snapshot;
	}

	void ApplySoldier(AActor& Soldier, const FOutbreakSoldierSnapshot& Snapshot)
	{
		if (!Soldier.HasAuthority())
		{
			return;
		}

		// Equipment first: a soldier that dies below still drops nothing extra, and inventory weight
		// counts loaded rounds (Phase 10).
		if (UOutbreakWeaponComponent* Weapons = Soldier.FindComponentByClass<UOutbreakWeaponComponent>())
		{
			TArray<FOutbreakWeaponSlot> Slots;
			int32 ActiveIndex = INDEX_NONE;
			for (int32 Index = 0; Index < Snapshot.Weapons.Num(); ++Index)
			{
				const FOutbreakSavedWeapon& Saved = Snapshot.Weapons[Index];
				UOutbreakWeaponDefinition* Weapon = LoadSaved(Saved.Weapon, Soldier);
				if (!Weapon)
				{
					continue;
				}
				FOutbreakWeaponSlot& Slot = Slots.AddDefaulted_GetRef();
				Slot.Definition = Weapon;
				Slot.LoadedAmmoType = LoadSaved(Saved.LoadedAmmoType, Soldier);
				Slot.SelectedAmmoType = LoadSaved(Saved.SelectedAmmoType, Soldier);
				Slot.AmmoInMagazine = Saved.AmmoInMagazine;
				Slot.FireModeIndex = Saved.FireModeIndex;
				if (Index == Snapshot.ActiveWeaponIndex)
				{
					ActiveIndex = Slots.Num() - 1;
				}
			}
			TArray<FOutbreakAmmoAmount> Reserves;
			for (const FOutbreakSavedAmmo& Saved : Snapshot.AmmoReserves)
			{
				if (UOutbreakAmmoTypeDefinition* AmmoType = LoadSaved(Saved.AmmoType, Soldier))
				{
					FOutbreakAmmoAmount& Reserve = Reserves.AddDefaulted_GetRef();
					Reserve.AmmoType = AmmoType;
					Reserve.Count = FMath::Max(0, Saved.Count);
				}
			}
			Weapons->RestoreSavedLoadout(Slots, ActiveIndex, Reserves);
		}

		if (UOutbreakInventoryComponent* Inventory = Soldier.FindComponentByClass<UOutbreakInventoryComponent>())
		{
			Inventory->ClearItems();
			for (const FOutbreakSavedItem& Saved : Snapshot.Items)
			{
				UOutbreakItemDefinition* Item = LoadSaved(Saved.Item, Soldier);
				if (!Item || Saved.Count <= 0)
				{
					continue;
				}
				const int32 Added = Inventory->AddItem(Item, Saved.Count);
				if (Added < Saved.Count)
				{
					UE_LOG(LogOutbreak, Warning, TEXT("%s: restored %d of %d %s (capacity or stack limit)."),
						*Soldier.GetName(), Added, Saved.Count, *Item->GetName());
				}
			}
		}

		if (UOutbreakHealthComponent* Health = Soldier.FindComponentByClass<UOutbreakHealthComponent>())
		{
			// A soldier down at the checkpoint gets back up as if revived, so loading never starts in a bleed-out.
			const float ReviveFraction = GetDefault<UOutbreakMedicalSettings>()->ReviveHealthFraction;
			const float Fraction = Snapshot.HealthFraction > 0.0f ? Snapshot.HealthFraction : ReviveFraction;
			Health->RestoreSavedState(Fraction, Snapshot.ReviveCount, Snapshot.bDead);
		}
	}

	FOutbreakSquadSnapshot CaptureSquad(const UOutbreakSquadComponent& Squad, const FOutbreakDeploymentSelection& Selection)
	{
		FOutbreakSquadSnapshot Snapshot;
		Snapshot.SquadSize = Selection.SquadSize;
		Snapshot.TeammateRosterIndices = Selection.TeammateRosterIndices;
		for (UOutbreakLoadoutDefinition* Loadout : Selection.MemberLoadouts)
		{
			Snapshot.MemberLoadouts.Add(Loadout);
		}

		const UOutbreakSquadMemberComponent* PlayerMember = Squad.GetPlayerMember();
		const AActor* PlayerActor = PlayerMember ? PlayerMember->GetOwner() : nullptr;
		Snapshot.Members.Add(PlayerActor ? CaptureSoldier(*PlayerActor) : FOutbreakSoldierSnapshot());
		for (const UOutbreakSquadMemberComponent* Teammate : Squad.GetTeammates())
		{
			const AActor* Owner = Teammate ? Teammate->GetOwner() : nullptr;
			if (Owner)
			{
				Snapshot.Members.Add(CaptureSoldier(*Owner));
			}
		}
		return Snapshot;
	}

	FOutbreakDeploymentSelection ToSelection(const FOutbreakSquadSnapshot& Snapshot)
	{
		FOutbreakDeploymentSelection Selection;
		Selection.SquadSize = Snapshot.SquadSize;
		Selection.TeammateRosterIndices = Snapshot.TeammateRosterIndices;
		for (const TSoftObjectPtr<UOutbreakLoadoutDefinition>& Loadout : Snapshot.MemberLoadouts)
		{
			// A missing loadout falls back to default equipment; the member's saved state replaces it anyway.
			Selection.MemberLoadouts.Add(Loadout.IsNull() ? nullptr : Loadout.LoadSynchronous());
		}
		return Selection;
	}

	int32 ApplySquad(UOutbreakSquadComponent& Squad, const FOutbreakSquadSnapshot& Snapshot)
	{
		TArray<AActor*> Soldiers;
		const UOutbreakSquadMemberComponent* PlayerMember = Squad.GetPlayerMember();
		Soldiers.Add(PlayerMember ? PlayerMember->GetOwner() : nullptr);
		for (const UOutbreakSquadMemberComponent* Teammate : Squad.GetTeammates())
		{
			Soldiers.Add(Teammate ? Teammate->GetOwner() : nullptr);
		}

		if (Soldiers.Num() != Snapshot.Members.Num())
		{
			UE_LOG(LogOutbreak, Warning, TEXT("Checkpoint squad has %d members, the rebuilt squad %d; restoring the ones that match."),
				Snapshot.Members.Num(), Soldiers.Num());
		}

		int32 Restored = 0;
		const int32 Count = FMath::Min(Soldiers.Num(), Snapshot.Members.Num());
		for (int32 Index = 0; Index < Count; ++Index)
		{
			if (AActor* Soldier = Soldiers[Index])
			{
				ApplySoldier(*Soldier, Snapshot.Members[Index]);
				++Restored;
			}
		}
		return Restored;
	}
}
