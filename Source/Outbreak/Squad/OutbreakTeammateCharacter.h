#pragma once

#include "CoreMinimal.h"
#include "Characters/OutbreakCharacter.h"

#include "OutbreakTeammateCharacter.generated.h"

class UOutbreakTeammateAIConfig;

/**
 * AI-controlled squad soldier. Shares movement, health, weapons and squad membership with the
 * player through AOutbreakCharacter, and is driven by AOutbreakSquadAIController.
 */
UCLASS(Blueprintable)
class OUTBREAK_API AOutbreakTeammateCharacter : public AOutbreakCharacter
{
	GENERATED_BODY()

public:

	AOutbreakTeammateCharacter();

	const UOutbreakTeammateAIConfig* GetTeammateAIConfig() const { return TeammateAIConfig; }

protected:

	/** Perception, priorities, following, combat and the Behavior Tree. Empty = class defaults, without a tree. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UOutbreakTeammateAIConfig> TeammateAIConfig;
};
