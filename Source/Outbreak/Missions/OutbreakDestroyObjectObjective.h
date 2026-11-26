#pragma once

#include "CoreMinimal.h"
#include "Missions/OutbreakEliminateTargetObjective.h"

#include "OutbreakDestroyObjectObjective.generated.h"

/**
 * Completes when every actor tagged TargetTag (at activation) is destroyed: its health component
 * is dead or the actor is gone. Tag AOutbreakDestructibleObject actors (relays, fuel tanks, nests)
 * in the level. The mechanics are Eliminate Target's; the class exists so missions read clearly
 * and object targets can diverge from character targets later.
 */
UCLASS(DisplayName = "Destroy Object")
class OUTBREAK_API UOutbreakDestroyObjectObjective : public UOutbreakEliminateTargetObjective
{
	GENERATED_BODY()

public:

	virtual bool Validate(TArray<FText>& OutErrors) const override;
};
