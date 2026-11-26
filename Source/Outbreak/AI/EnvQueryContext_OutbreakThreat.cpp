#include "AI/EnvQueryContext_OutbreakThreat.h"

#include "AI/OutbreakAIAgent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameFramework/Pawn.h"

void UEnvQueryContext_OutbreakThreat::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	const APawn* Querier = Cast<APawn>(QueryInstance.Owner.Get());
	const IOutbreakAIAgent* Agent = Querier ? Cast<IOutbreakAIAgent>(Querier->GetController()) : nullptr;

	FVector ThreatLocation;
	if (Agent && Agent->GetCoverThreatLocation(ThreatLocation))
	{
		UEnvQueryItemType_Point::SetContextHelper(ContextData, ThreatLocation);
	}
}
