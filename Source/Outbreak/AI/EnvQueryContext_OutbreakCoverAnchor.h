#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"

#include "EnvQueryContext_OutbreakCoverAnchor.generated.h"

/**
 * EQS context: where cover should be found around (IOutbreakAIAgent::GetCoverAnchorLocation):
 * the defended position for Defend, the ordered position for Cover, otherwise the querier itself.
 * Use it as the generator centre and for "distance to anchor" scoring.
 */
UCLASS(DisplayName = "Outbreak: Cover Anchor")
class OUTBREAK_API UEnvQueryContext_OutbreakCoverAnchor : public UEnvQueryContext
{
	GENERATED_BODY()

public:

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
