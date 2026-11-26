#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"

#include "BTService_OutbreakUpdateBehavior.generated.h"

/**
 * Runs IOutbreakAIAgent::UpdateBehavior at the service interval (teammates and enemies):
 * perception, state or behaviour selection and destination, written to the blackboard.
 */
UCLASS(DisplayName = "Outbreak: Update Behavior")
class OUTBREAK_API UBTService_OutbreakUpdateBehavior : public UBTService
{
	GENERATED_BODY()

public:

	UBTService_OutbreakUpdateBehavior();

protected:

	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
