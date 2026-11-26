#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"

#include "BTService_OutbreakTeammateCombat.generated.h"

/**
 * Teammate combat at the service interval: aims at the controller's combat target, fires when
 * aimed and in range, and reloads automatically (empty, or low while no target is present).
 * Uses the same weapon component requests as the player, so the server validates every shot.
 */
UCLASS(DisplayName = "Outbreak: Teammate Combat")
class OUTBREAK_API UBTService_OutbreakTeammateCombat : public UBTService
{
	GENERATED_BODY()

public:

	UBTService_OutbreakTeammateCombat();

protected:

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
