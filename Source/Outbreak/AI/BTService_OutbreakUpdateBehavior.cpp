#include "AI/BTService_OutbreakUpdateBehavior.h"

#include "AI/OutbreakSquadAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTService_OutbreakUpdateBehavior::UBTService_OutbreakUpdateBehavior()
{
	NodeName = TEXT("Outbreak: Update Teammate Behavior");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;
}

void UBTService_OutbreakUpdateBehavior::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	if (AOutbreakSquadAIController* Controller = Cast<AOutbreakSquadAIController>(OwnerComp.GetAIOwner()))
	{
		Controller->UpdateBehavior();
	}
}

void UBTService_OutbreakUpdateBehavior::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	if (AOutbreakSquadAIController* Controller = Cast<AOutbreakSquadAIController>(OwnerComp.GetAIOwner()))
	{
		Controller->UpdateBehavior();
	}
}
