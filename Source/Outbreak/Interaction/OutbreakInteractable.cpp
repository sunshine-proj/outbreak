#include "Interaction/OutbreakInteractable.h"

bool IOutbreakInteractable::CanInteract_Implementation(AActor* InteractionInstigator) const
{
	return true;
}

FText IOutbreakInteractable::GetInteractionPrompt_Implementation(AActor* InteractionInstigator) const
{
	return NSLOCTEXT("OutbreakInteraction", "DefaultPrompt", "Interact");
}

void IOutbreakInteractable::Interact_Implementation(AActor* InteractionInstigator)
{
}
