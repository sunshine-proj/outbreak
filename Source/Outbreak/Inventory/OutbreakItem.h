#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "OutbreakItem.generated.h"

/** What kind of thing an item is. Decides where it is stored and how it is used. */
UENUM(BlueprintType)
enum class EOutbreakItemCategory : uint8
{
	/** A UOutbreakWeaponDefinition: goes into an equipment slot (the weapon component). */
	Weapon,
	/** A UOutbreakAmmoTypeDefinition: goes into the ammunition reserve (the weapon component). */
	Ammunition,
	/** Medkits and the like: heal (UOutbreakItemDefinition with a heal action). */
	Medical,
	/** Grenades, flares, tools: used from the utility slot. */
	Utility,
	/** Mission-specific objects (keycards, data cores). Not droppable by default. */
	Mission
};

/** Which equipment slot a weapon occupies (Phase 10). */
UENUM(BlueprintType)
enum class EOutbreakEquipmentSlot : uint8
{
	Primary,
	Secondary
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UOutbreakItem : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything a soldier can carry: weapon definitions, ammunition types and UOutbreakItemDefinition
 * assets (medical, utility, mission items). The inventory weighs and stacks items through this
 * interface; pickups hold any object implementing it.
 */
class OUTBREAK_API IOutbreakItem
{
	GENERATED_BODY()

public:

	virtual FText GetItemDisplayName() const = 0;

	virtual EOutbreakItemCategory GetItemCategory() const = 0;

	/** Weight of one unit (one weapon, one round, one medkit), in kilograms. */
	virtual float GetItemUnitWeight() const = 0;

	/** Most units carried in one stack. 0 = unlimited (only weight limits it). */
	virtual int32 GetItemMaxStack() const { return 0; }
};
