#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"

#include "BTService_OutbreakCombat.generated.h"

/**
 * AI combat at the service interval, for any IOutbreakAIAgent (teammates and enemies): aims at the
 * agent's combat target, fires when aimed and in range, and reloads automatically (empty, or low
 * while no target is present).
 * Uses the same weapon component requests as the player, so the server validates every shot.
 */
UCLASS(DisplayName = "Outbreak: Combat")
class OUTBREAK_API UBTService_OutbreakCombat : public UBTService
{
	GENERATED_BODY()

public:

	UBTService_OutbreakCombat();

protected:

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
