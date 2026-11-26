#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/OutbreakInteractable.h"

#include "OutbreakMissionEquipment.generated.h"

class AOutbreakMissionEquipment;
class UOutbreakItemDefinition;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOutbreakEquipmentActivatedSignature, AOutbreakMissionEquipment*, Equipment, AActor*, ActivatedBy);

/**
 * Equipment a soldier switches on with Interact, for Activate Equipment objectives: a terminal, an
 * uplink, a generator, a beacon.
 *
 * Starts locked when bStartsLocked; the objective that needs it unlocks it when it becomes active,
 * so it cannot be used out of order. RequiredItem (a keycard, a power cell) must be carried, and is
 * used up when bConsumeRequiredItem. Activation is instant and permanent. Replicated; server-authoritative.
 */
UCLASS(Blueprintable)
class OUTBREAK_API AOutbreakMissionEquipment : public AActor, public IOutbreakInteractable
{
	GENERATED_BODY()

public:

	AOutbreakMissionEquipment();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsActivated() const { return bActivated; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsLocked() const { return bLocked; }

	/** Authority. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	void SetLocked(bool bNewLocked);

	/** Authority: what must be carried to activate it (null = nothing), and whether activating uses it up. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	void SetRequiredItem(UOutbreakItemDefinition* NewRequiredItem, bool bConsume);

	/** Authority: activates without the item check (scripted activation). Returns false if already active. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	bool Activate(AActor* ActivatedBy);

	/** Fires on every machine when the equipment is activated. ActivatedBy is null on clients. */
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOutbreakEquipmentActivatedSignature OnActivated;

protected:

	virtual void PostInitializeComponents() override;

	//~ Begin IOutbreakInteractable interface
	virtual bool CanInteract_Implementation(AActor* InteractionInstigator) const override;
	virtual FText GetInteractionPrompt_Implementation(AActor* InteractionInstigator) const override;
	virtual void Interact_Implementation(AActor* InteractionInstigator) override;
	//~ End IOutbreakInteractable interface

	/** For Blueprint feedback (lights, sounds) on every machine. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment")
	void OnEquipmentActivated();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Prompt while it can be activated, e.g. "Activate uplink". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FText ActivatePrompt;

	/** Locked until its objective becomes active. Off = usable at any time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	bool bStartsLocked = true;

	/** Must be carried to activate. Empty = nothing needed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UOutbreakItemDefinition> RequiredItem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "RequiredItem != nullptr"))
	bool bConsumeRequiredItem = true;

private:

	bool HasRequiredItem(const AActor* InteractionInstigator) const;

	UFUNCTION()
	void OnRep_Activated();

	UPROPERTY(ReplicatedUsing = OnRep_Activated)
	bool bActivated = false;

	UPROPERTY(Replicated)
	bool bLocked = false;
};
