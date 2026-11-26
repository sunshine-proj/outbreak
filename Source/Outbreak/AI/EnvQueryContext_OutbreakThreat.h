#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"

#include "EnvQueryContext_OutbreakThreat.generated.h"

/**
 * EQS context: the location of the threat the querier wants cover from
 * (IOutbreakAIAgent::GetCoverThreatLocation). Provides nothing when no threat is known, which
 * makes visibility tests against it pass trivially; cover queries should only run with a threat.
 */
UCLASS(DisplayName = "Outbreak: Threat")
class OUTBREAK_API UEnvQueryContext_OutbreakThreat : public UEnvQueryContext
{
	GENERATED_BODY()

public:

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
