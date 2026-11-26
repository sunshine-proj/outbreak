#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Weapons/OutbreakWeaponTypes.h"

#include "OutbreakSquadRoleDefinition.generated.h"

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

	/** Distance this role prefers to fight at. Read by teammate AI (Phase 4). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Preferences", meta = (ClampMin = "0.0", Units = "cm"))
	float PreferredEngagementRange = 1500.0f;
};
