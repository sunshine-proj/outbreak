#pragma once

#include "CoreMinimal.h"
#include "Missions/OutbreakObjective.h"

#include "OutbreakAvoidAlarmObjective.generated.h"

/**
 * "Stay undetected" (Phase 14 complex objectives): fails when the alarm goes up, completes when the
 * objective UntilObjectiveId completes. Usually optional, alongside the main line (Activation At
 * Mission Start). Fails too if that objective fails.
 */
UCLASS(DisplayName = "Avoid Alarm")
class OUTBREAK_API UOutbreakAvoidAlarmObjective : public UOutbreakObjective
{
	GENERATED_BODY()

public:

	virtual EOutbreakObjectiveState Evaluate(UOutbreakMissionSubsystem& Mission) override;
	virtual bool Validate(TArray<FText>& OutErrors) const override;
	virtual bool ValidateReferences(const TSet<FName>& ObjectiveIds, TArray<FText>& OutErrors) const override;

	/** ObjectiveId of the objective that ends the stealth requirement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName UntilObjectiveId;
};
