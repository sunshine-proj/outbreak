#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"

#include "OutbreakDamageTypes.generated.h"

/**
 * Base class for Outbreak damage types. Damage types are classes, so health components can
 * resist a whole family of them (see UOutbreakHealthComponent::DamageMultipliers).
 *
 * Ammunition picks its damage type in UOutbreakAmmoTypeDefinition. Add new damage types as
 * Blueprint or C++ subclasses; nothing in the damage code needs to change.
 */
UCLASS(Blueprintable)
class OUTBREAK_API UOutbreakDamageType : public UDamageType
{
	GENERATED_BODY()
};

/** Standard kinetic rounds. */
UCLASS()
class OUTBREAK_API UOutbreakBallisticDamageType : public UOutbreakDamageType
{
	GENERATED_BODY()
};

/** Armour-piercing rounds. A sibling of ballistic, so armour can treat it differently. */
UCLASS()
class OUTBREAK_API UOutbreakArmourPiercingDamageType : public UOutbreakDamageType
{
	GENERATED_BODY()
};

/** Explosions and explosive rounds. */
UCLASS()
class OUTBREAK_API UOutbreakExplosiveDamageType : public UOutbreakDamageType
{
	GENERATED_BODY()
};

/** Energy weapons. */
UCLASS()
class OUTBREAK_API UOutbreakEnergyDamageType : public UOutbreakDamageType
{
	GENERATED_BODY()
};
