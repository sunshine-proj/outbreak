#include "AI/EnvQueryContext_OutbreakCoverAnchor.h"

#include "AI/OutbreakAIAgent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameFramework/Pawn.h"

void UEnvQueryContext_OutbreakCoverAnchor::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	const APawn* Querier = Cast<APawn>(QueryInstance.Owner.Get());
	if (!Querier)
	{
		return;
	}

	const IOutbreakAIAgent* Agent = Cast<IOutbreakAIAgent>(Querier->GetController());
	FVector Anchor = Querier->GetActorLocation();
	if (Agent)
	{
		Agent->GetCoverAnchorLocation(Anchor);
	}
	UEnvQueryItemType_Point::SetContextHelper(ContextData, Anchor);
}
