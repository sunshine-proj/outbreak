#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"

#include "BTService_OutbreakUpdateBehavior.generated.h"

/**
 * Runs AOutbreakSquadAIController::UpdateBehavior at the service interval: threat tracking,
 * order progress and behaviour selection, written to the blackboard.
 */
UCLASS(DisplayName = "Outbreak: Update Teammate Behavior")
class OUTBREAK_API UBTService_OutbreakUpdateBehavior : public UBTService
{
	GENERATED_BODY()

public:

	UBTService_OutbreakUpdateBehavior();

protected:

	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
