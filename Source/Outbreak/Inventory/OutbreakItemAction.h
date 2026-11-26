#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "OutbreakItemAction.generated.h"

class AActor;
class AOutbreakProjectile;
class UOutbreakAmmoTypeDefinition;

/** Who uses an item on whom. Built by UOutbreakInventoryComponent on the authority. */
struct FOutbreakItemUseContext
{
	AActor* User = nullptr;

	/** The user itself, or another soldier (healing a squadmate). */
	AActor* Target = nullptr;

	/** Scales the effect (the user's role, for example a medic heals more). */
	float EffectMultiplier = 1.0f;
};

/**
 * What using an item does. Instanced inside a UOutbreakItemDefinition. Runs on the authority only.
 * Implementations must be stateless: one instance is shared by every holder of the item.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories, Const)
class OUTBREAK_API UOutbreakItemAction : public UObject
{
	GENERATED_BODY()

public:

	/** Whether the item can be used in Context right now (for example: the target is hurt). */
	virtual bool CanUse(const FOutbreakItemUseContext& Context) const { return Context.User != nullptr; }

	/** Performs the action. Returns true if one unit of the item was consumed. */
	virtual bool Use(const FOutbreakItemUseContext& Context) const PURE_VIRTUAL(UOutbreakItemAction::Use, return false;);

	/** Whether the item may be used on someone other than the user. */
	virtual bool CanTargetOthers() const { return false; }
};

/** Restores health to a living, hurt soldier (medkits, stims). */
UCLASS(DisplayName = "Heal")
class OUTBREAK_API UOutbreakHealItemAction : public UOutbreakItemAction
{
	GENERATED_BODY()

public:

	virtual bool CanUse(const FOutbreakItemUseContext& Context) const override;
	virtual bool Use(const FOutbreakItemUseContext& Context) const override;
	virtual bool CanTargetOthers() const override { return bCanUseOnOthers; }

	/** Health restored, before the user's medical multiplier. */
	UPROPERTY(EditAnywhere, Category = "Heal", meta = (ClampMin = "1.0"))
	float HealAmount = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Heal")
	bool bCanUseOnOthers = true;
};

/** Throws a projectile from the user's eyes (grenades, flares). */
UCLASS(DisplayName = "Throw")
class OUTBREAK_API UOutbreakThrowItemAction : public UOutbreakItemAction
{
	GENERATED_BODY()

public:

	UOutbreakThrowItemAction();

	virtual bool Use(const FOutbreakItemUseContext& Context) const override;

	UPROPERTY(EditAnywhere, Category = "Throw")
	TSubclassOf<AOutbreakProjectile> ProjectileClass;

	/** Damage type, explosion radius and impact effects. */
	UPROPERTY(EditAnywhere, Category = "Throw")
	TObjectPtr<UOutbreakAmmoTypeDefinition> AmmoType;

	UPROPERTY(EditAnywhere, Category = "Throw", meta = (ClampMin = "0.0"))
	float Damage = 120.0f;

	UPROPERTY(EditAnywhere, Category = "Throw", meta = (ClampMin = "1.0", Units = "cm/s"))
	float ThrowSpeed = 1500.0f;

	UPROPERTY(EditAnywhere, Category = "Throw")
	float GravityScale = 1.0f;

	/** Added to the view pitch so throws arc. */
	UPROPERTY(EditAnywhere, Category = "Throw", meta = (Units = "deg"))
	float LaunchPitchOffset = 10.0f;
};
