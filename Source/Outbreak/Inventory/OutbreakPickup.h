#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/OutbreakInteractable.h"

#include "OutbreakPickup.generated.h"

class UStaticMeshComponent;

/**
 * An item lying in the world (Phase 10 item collection): a weapon, ammunition, a medkit, a grenade,
 * a mission object. Placed by designers or spawned when a soldier drops something.
 *
 * Interacting puts as much as fits into the interacting soldier's inventory. What does not fit
 * stays. An empty pickup removes itself. A weapon whose slot is taken is swapped: the carried one is
 * left here in its place.
 */
UCLASS(Blueprintable)
class OUTBREAK_API AOutbreakPickup : public AActor, public IOutbreakInteractable
{
	GENERATED_BODY()

public:

	AOutbreakPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Authority. Item must implement IOutbreakItem (weapon, ammunition or item definition). */
	void SetContents(UObject* NewItem, int32 NewCount);

	UFUNCTION(BlueprintPure, Category = "Pickup")
	UObject* GetItem() const { return Item; }

	UFUNCTION(BlueprintPure, Category = "Pickup")
	int32 GetCount() const { return Count; }

protected:

	//~ Begin IOutbreakInteractable interface
	virtual bool CanInteract_Implementation(AActor* InteractionInstigator) const override;
	virtual FText GetInteractionPrompt_Implementation(AActor* InteractionInstigator) const override;
	virtual void Interact_Implementation(AActor* InteractionInstigator) override;
	//~ End IOutbreakInteractable interface

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** A weapon definition, ammunition type or item definition. */
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Pickup", meta = (AllowedClasses = "/Script/Outbreak.OutbreakWeaponDefinition,/Script/Outbreak.OutbreakAmmoTypeDefinition,/Script/Outbreak.OutbreakItemDefinition"))
	TObjectPtr<UObject> Item;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = "1"))
	int32 Count = 1;
};
