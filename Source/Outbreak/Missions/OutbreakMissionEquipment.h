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
 * Interactable equipment a soldier switches on with Interact: a terminal, an uplink, a generator, a
 * light switch, a door control. Used by Activate Equipment objectives (Phase 11) and as level
 * equipment (Phase 12).
 *
 * Starts locked when bStartsLocked; the objective that needs it unlocks it when it becomes active,
 * so it cannot be used out of order. RequiredItem (a keycard, a power cell) must be carried the
 * first time it is switched on, and is used up when bConsumeRequiredItem. Activation is instant;
 * with bCanToggle it can be switched off again.
 *
 * Linked Actors follow it: actors implementing IOutbreakSwitchable (doors) are switched on and off
 * with it; other actors' light components are turned on or off, which also changes the stealth
 * light level. Using it makes a noise. Replicated; server-authoritative.
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

	/** Authority: switches it off (only meaningful with bCanToggle for players; scripts may always). Returns false if already off. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	bool Deactivate(AActor* DeactivatedBy);

	/** Authority: whether players can switch it off again. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	void SetCanToggle(bool bInCanToggle) { bCanToggle = bInCanToggle; }

	/** Authority: the actors that follow this equipment (normally set in the level). Applies the current state to their lights. */
	void SetLinkedActors(const TArray<AActor*>& InLinkedActors);

	/** Fires on every machine when the equipment is activated. ActivatedBy is null on clients. */
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOutbreakEquipmentActivatedSignature OnActivated;

	/** Fires on every machine when the equipment is switched off. DeactivatedBy is null on clients. */
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOutbreakEquipmentActivatedSignature OnDeactivated;

protected:

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

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

	/** Can be switched off again with Interact (a light switch). Off = stays on once activated (an uplink). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	bool bCanToggle = false;

	/** Prompt while it can be switched off. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "bCanToggle"))
	FText DeactivatePrompt;

	/** Doors and other IOutbreakSwitchable actors switch with it; other actors' lights turn on or off. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Linked")
	TArray<TObjectPtr<AActor>> LinkedActors;

	/** Linked lights are on while this is on (a generator). Off = linked lights are on while this is off (a breaker that cuts them). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Linked")
	bool bLinkedLightsOnWhenActive = true;

	/** Gameplay noise of switching it (0 = silent). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "0.0"))
	float UseNoiseLoudness = 0.3f;

private:

	bool HasRequiredItem(const AActor* InteractionInstigator) const;

	/** Lights on every machine; switchable actors on the authority. */
	void ApplyLinkedActors(AActor* SwitchedBy, bool bIncludeSwitchables);
	void ReportUseNoise(AActor* SwitchedBy) const;

	UFUNCTION()
	void OnRep_Activated();

	UPROPERTY(ReplicatedUsing = OnRep_Activated)
	bool bActivated = false;

	UPROPERTY(Replicated)
	bool bLocked = false;

	/** The required item has been handed over; switching on again does not need it. */
	bool bRequiredItemUsed = false;
};
