#pragma once

#include "CoreMinimal.h"
#include "Characters/OutbreakCharacter.h"

#include "OutbreakTeammateCharacter.generated.h"

/**
 * AI-controlled squad soldier. Shares movement, health, weapons and squad membership with the
 * player through AOutbreakCharacter.
 *
 * Phase 3 provides the character only: it is possessed by a plain AAIController and stands
 * still. The squad AI controller and behaviour arrive in Phase 4.
 */
UCLASS(Blueprintable)
class OUTBREAK_API AOutbreakTeammateCharacter : public AOutbreakCharacter
{
	GENERATED_BODY()

public:

	AOutbreakTeammateCharacter();
};
