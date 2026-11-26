#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/OutbreakInteractable.h"

#include "OutbreakTestInteractableActor.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOutbreakTestInteractableToggledSignature,
	AActor*, InteractionInstigator, bool, bIsActive);

/**
 * Test and greybox interactable: a cube that toggles between active and inactive.
 *
 * Used to verify interaction detection, range and the HUD prompt. Not intended as shipping content.
 */
UCLASS(Blueprintable)
class OUTBREAK_API AOutbreakTestInteractableActor : public AActor, public IOutbreakInteractable
{
	GENERATED_BODY()

public:

	AOutbreakTestInteractableActor();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsActive() const { return bIsActive; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	int32 GetInteractionCount() const { return InteractionCount; }

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionEnabled(bool bEnabled) { bInteractionEnabled = bEnabled; }

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOutbreakTestInteractableToggledSignature OnToggled;

protected:

	//~ Begin IOutbreakInteractable interface
	virtual bool CanInteract_Implementation(AActor* InteractionInstigator) const override;
	virtual FText GetInteractionPrompt_Implementation(AActor* InteractionInstigator) const override;
	virtual void Interact_Implementation(AActor* InteractionInstigator) override;
	//~ End IOutbreakInteractable interface

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText ActivatePrompt;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText DeactivatePrompt;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bInteractionEnabled = true;

private:

	bool bIsActive = false;
	int32 InteractionCount = 0;
};
