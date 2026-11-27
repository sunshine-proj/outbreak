#include "AI/BTService_OutbreakUpdateBehavior.h"

#include "AI/OutbreakAIAgent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Outbreak.h"

// Phase 15 profiling: `stat Outbreak`, Insights and the Outbreak CSV category.
DECLARE_CYCLE_STAT(TEXT("AI behaviour service (teammates and enemies)"), STAT_OutbreakBehaviorService, STATGROUP_Outbreak);

UBTService_OutbreakUpdateBehavior::UBTService_OutbreakUpdateBehavior()
{
	NodeName = TEXT("Outbreak: Update Behavior");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;
}

void UBTService_OutbreakUpdateBehavior::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	if (IOutbreakAIAgent* Agent = Cast<IOutbreakAIAgent>(OwnerComp.GetAIOwner()))
	{
		Agent->UpdateBehavior();
	}
}

void UBTService_OutbreakUpdateBehavior::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	OUTBREAK_SCOPE_CYCLE_COUNTER(STAT_OutbreakBehaviorService);

	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	if (IOutbreakAIAgent* Agent = Cast<IOutbreakAIAgent>(OwnerComp.GetAIOwner()))
	{
		Agent->UpdateBehavior();
	}
}
