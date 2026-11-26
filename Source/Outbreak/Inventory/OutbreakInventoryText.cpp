#include "Inventory/OutbreakInventoryText.h"

#include "Combat/OutbreakHealthComponent.h"
#include "Combat/OutbreakMedicalComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Inventory/OutbreakItemDefinition.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

#define LOCTEXT_NAMESPACE "OutbreakInventoryText"

namespace
{
	FText FormatWeight(const UOutbreakInventoryComponent* Inventory)
	{
		FNumberFormattingOptions OneDecimal;
		OneDecimal.MinimumFractionalDigits = 1;
		OneDecimal.MaximumFractionalDigits = 1;
		return FText::Format(LOCTEXT("Weight", "{0} / {1} kg"),
			FText::AsNumber(Inventory->GetCarriedWeight(), &OneDecimal),
			FText::AsNumber(Inventory->GetMaxCarryWeight(), &OneDecimal));
	}

	FText DisplayNameOf(const AActor* Actor)
	{
		const UOutbreakSquadMemberComponent* Member = Actor ? Actor->FindComponentByClass<UOutbreakSquadMemberComponent>() : nullptr;
		return Member && !Member->GetDisplayName().IsEmpty() ? Member->GetDisplayName() : FText::FromString(GetNameSafe(Actor));
	}
}

namespace OutbreakInventoryText
{
	FText BuildInventoryText(const UOutbreakInventoryComponent* Inventory, bool bExpanded)
	{
		if (!Inventory)
		{
			return FText::GetEmpty();
		}

		const UOutbreakItemDefinition* SelectedUtility = Inventory->GetSelectedUtility();
		TArray<FText> ItemParts;
		for (const FOutbreakItemAmount& Entry : Inventory->GetItems())
		{
			if (!Entry.Item || Entry.Count <= 0)
			{
				continue;
			}
			const FText Part = FText::Format(LOCTEXT("ItemCount", "{0} x{1}"), Entry.Item->DisplayName, Entry.Count);
			ItemParts.Add(Entry.Item.Get() == SelectedUtility ? FText::Format(LOCTEXT("Selected", "[{0}]"), Part) : Part);
		}

		if (!bExpanded)
		{
			TArray<FText> Parts = ItemParts;
			Parts.Add(FormatWeight(Inventory));
			return FText::Join(FText::FromString(TEXT("  |  ")), Parts);
		}

		const auto SlotLine = [Inventory](EOutbreakEquipmentSlot Slot, const FText& Label)
		{
			const UOutbreakWeaponDefinition* Weapon = Inventory->GetWeaponInSlot(Slot);
			return FText::Format(LOCTEXT("SlotLine", "{0}: {1}"), Label, Weapon ? Weapon->DisplayName : LOCTEXT("Empty", "-"));
		};

		TArray<FText> Lines;
		Lines.Add(LOCTEXT("Header", "INVENTORY"));
		Lines.Add(SlotLine(EOutbreakEquipmentSlot::Primary, LOCTEXT("Primary", "Primary")));
		Lines.Add(SlotLine(EOutbreakEquipmentSlot::Secondary, LOCTEXT("Secondary", "Secondary")));

		const AActor* Owner = Inventory->GetOwner();
		if (const UOutbreakWeaponComponent* Weapons = Owner ? Owner->FindComponentByClass<UOutbreakWeaponComponent>() : nullptr)
		{
			for (const FOutbreakAmmoAmount& Reserve : Weapons->GetAmmoReserves())
			{
				if (Reserve.AmmoType && Reserve.Count > 0)
				{
					Lines.Add(FText::Format(LOCTEXT("AmmoLine", "Ammo: {0} x{1}"), Reserve.AmmoType->DisplayName, Reserve.Count));
				}
			}
		}
		for (const FText& Part : ItemParts)
		{
			Lines.Add(Part);
		}
		Lines.Add(FText::Format(LOCTEXT("Carried", "Carried: {0}"), FormatWeight(Inventory)));
		return FText::Join(FText::FromString(TEXT("\n")), Lines);
	}

	FText BuildMedicalStatusText(const AActor* Soldier)
	{
		const UOutbreakHealthComponent* Health = Soldier ? Soldier->FindComponentByClass<UOutbreakHealthComponent>() : nullptr;
		if (!Health)
		{
			return FText::GetEmpty();
		}

		if (Health->IsIncapacitated())
		{
			if (const UOutbreakMedicalComponent* Reviver = UOutbreakMedicalComponent::FindReviverOf(Soldier->GetWorld(), Soldier))
			{
				return FText::Format(LOCTEXT("BeingRevived", "Being revived by {0} {1}%"),
					DisplayNameOf(Reviver->GetOwner()), FMath::RoundToInt(Reviver->GetReviveProgress() * 100.0f));
			}
			const float Remaining = Health->GetBleedOutTimeRemaining();
			return Remaining >= 0.0f
				? FText::Format(LOCTEXT("DownTimed", "DOWN - bleeding out in {0} s"), FMath::CeilToInt(Remaining))
				: LOCTEXT("Down", "DOWN - waiting for a revive");
		}

		const UOutbreakMedicalComponent* Medical = Soldier->FindComponentByClass<UOutbreakMedicalComponent>();
		if (Medical && Medical->IsReviving())
		{
			return FText::Format(LOCTEXT("Reviving", "Reviving {0} {1}%"),
				DisplayNameOf(Medical->GetRevivePatient()), FMath::RoundToInt(Medical->GetReviveProgress() * 100.0f));
		}
		return FText::GetEmpty();
	}
}

#undef LOCTEXT_NAMESPACE
