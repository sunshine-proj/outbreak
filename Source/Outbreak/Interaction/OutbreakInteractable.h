#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "OutbreakInteractable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UOutbreakInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by any actor the player (and later AI teammates) can interact with:
 * doors, terminals, pickups, objectives.
 *
 * Implementable in C++ or Blueprint. Call through the generated Execute_ functions, for example
 * IOutbreakInteractable::Execute_Interact(Actor, Instigator).
 */
class OUTBREAK_API IOutbreakInteractable
{
	GENERATED_BODY()

public:

	/** Whether InteractionInstigator may interact right now. Defaults to true. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* InteractionInstigator) const;

	/** Short action text for the HUD prompt, for example "Open door". */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPrompt(AActor* InteractionInstigator) const;

	/** Performs the interaction. Only called after CanInteract returned true. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* InteractionInstigator);

protected:

	virtual bool CanInteract_Implementation(AActor* InteractionInstigator) const;
	virtual FText GetInteractionPrompt_Implementation(AActor* InteractionInstigator) const;
	virtual void Interact_Implementation(AActor* InteractionInstigator);
};
