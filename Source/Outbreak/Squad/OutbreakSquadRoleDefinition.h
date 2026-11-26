#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Weapons/OutbreakWeaponTypes.h"

#include "OutbreakSquadRoleDefinition.generated.h"

class UOutbreakLoadoutDefinition;
class UOutbreakVoiceDefinition;
class UOutbreakWeaponDefinition;

/**
 * A tactical role (Rifleman, Assault, Support, Sniper, Medic, Scout, ...).
 *
 * Roles are gameplay configuration, not character classes (PRD §5.2): any soldier can take any
 * role. A role supplies default equipment and the preferences AI behaviour reads from Phase 4.
 */
UCLASS(BlueprintType, Const)
class OUTBREAK_API UOutbreakSquadRoleDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Role")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Role", meta = (MultiLine = "true"))
	FText Description;

	/** Weapons given to members with this role unless their member configuration overrides them. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TArray<TObjectPtr<UOutbreakWeaponDefinition>> DefaultWeapons;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TArray<FOutbreakAmmoAmount> DefaultAmmoReserves;

	/** Medical, utility and mission items for members with this role (Phase 10). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TArray<FOutbreakItemAmount> DefaultItems;

	/** A complete loadout; when set it replaces DefaultWeapons, DefaultAmmoReserves and DefaultItems. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UOutbreakLoadoutDefinition> DefaultLoadout;

	/** Carrying capacity in kilograms. 0 = the character's own. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "0.0", Units = "kg"))
	float CarryCapacity = 0.0f;

	/** Scales health restored by medical items this member uses (a Medic heals more). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Medical", meta = (ClampMin = "0.0"))
	float MedicalEffectMultiplier = 1.0f;

	/** Scales revive speed (2 = revives in half the time). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Medical", meta = (ClampMin = "0.1"))
	float ReviveSpeedMultiplier = 1.0f;

	/** Distance this role prefers to fight at. Read by teammate AI (Phase 4). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Preferences", meta = (ClampMin = "0.0", Units = "cm"))
	float PreferredEngagementRange = 1500.0f;

	/** What members with this role say (Phase 13 squad communication). Empty = the project default voice. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Communication")
	TObjectPtr<UOutbreakVoiceDefinition> Voice;
};
