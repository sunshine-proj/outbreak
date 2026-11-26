#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Squad/OutbreakSquadTypes.h"
#include "Weapons/OutbreakWeaponTypes.h"
#include "Inventory/OutbreakInventoryComponent.h"

#include "OutbreakSquadConfiguration.generated.h"

class AOutbreakTeammateCharacter;
class UOutbreakLoadoutDefinition;
class UOutbreakSquadRoleDefinition;
class UOutbreakWeaponDefinition;

/** One AI teammate in a squad configuration. Empty fields fall back to the role or project defaults. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakSquadMemberConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Member")
	FText DisplayName;

	/** Character Blueprint (model, health, movement). Empty = project DefaultTeammateClass. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Member")
	TSubclassOf<AOutbreakTeammateCharacter> CharacterClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Member")
	TObjectPtr<UOutbreakSquadRoleDefinition> Role;

	/** Overrides the role's DefaultWeapons when not empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TArray<TObjectPtr<UOutbreakWeaponDefinition>> Weapons;

	/** Overrides the role's DefaultAmmoReserves when not empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TArray<FOutbreakAmmoAmount> AmmoReserves;

	/** Overrides the role's DefaultItems when not empty (Phase 10). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TArray<FOutbreakItemAmount> Items;

	/** A complete loadout; when set it overrides every other equipment field and the role's. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UOutbreakLoadoutDefinition> Loadout;
};

/**
 * A mission's squad: how many soldiers and who the AI teammates are.
 *
 * The player is always member one and is not listed. SquadSize - 1 teammates are created:
 * Teammates[0..] in order, then copies of FillerTeammate if the list is shorter than needed.
 * Teammates beyond SquadSize - 1 are ignored.
 */
UCLASS(BlueprintType, Const)
class OUTBREAK_API UOutbreakSquadConfiguration : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** Returns false and appends a reason for each invalid setting. Limits are checked when the squad is created. */
	bool Validate(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/** Total soldiers including the player. 0 = the resolved default size. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Size", meta = (ClampMin = "0"))
	int32 SquadSize = 0;

	/** Narrows the project minimum for this mission. 0 = project minimum. Cannot go below it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Size", meta = (ClampMin = "0"))
	int32 MinSquadSizeOverride = 0;

	/** Narrows the project maximum for this mission. 0 = project maximum. Cannot go above it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Size", meta = (ClampMin = "0"))
	int32 MaxSquadSizeOverride = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Members")
	TArray<FOutbreakSquadMemberConfig> Teammates;

	/** Used for teammate slots beyond the Teammates list. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Members")
	FOutbreakSquadMemberConfig FillerTeammate;

	/** The player's loadout. Empty = the player Blueprint's own weapons and items. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UOutbreakLoadoutDefinition> PlayerLoadout;

	/**
	 * Loadouts squad equipment selection may assign to any member, including the player
	 * (UOutbreakSquadComponent::SelectMemberLoadout). Empty = no selection.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TArray<TObjectPtr<UOutbreakLoadoutDefinition>> AvailableLoadouts;

	/** Formation at mission start. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Formation")
	EOutbreakFormation InitialFormation = EOutbreakFormation::Wedge;
};
