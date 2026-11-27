#pragma once

#include "CoreMinimal.h"
#include "Missions/OutbreakObjective.h"

#include "OutbreakCompositeObjective.generated.h"

/**
 * Completes when RequiredCount of its sub-objectives have completed (Phase 14 complex objectives):
 * alternative routes ("destroy the generator or hack the relay": 1 of 2), or several tasks done in
 * any order (0 = all). Fails as soon as too few sub-objectives are left to reach the count.
 *
 * Sub-objectives are any objective type; they all activate with the composite, and their own
 * dependency settings and optional flags are ignored. Their time limits apply.
 */
UCLASS(DisplayName = "Composite (N of)")
class OUTBREAK_API UOutbreakCompositeObjective : public UOutbreakObjective
{
	GENERATED_BODY()

public:

	virtual void Activate(UOutbreakMissionSubsystem& Mission) override;
	virtual EOutbreakObjectiveState Evaluate(UOutbreakMissionSubsystem& Mission) override;
	virtual void Deactivate() override;
	virtual bool Validate(TArray<FText>& OutErrors) const override;
	virtual bool ValidateReferences(const TSet<FName>& ObjectiveIds, TArray<FText>& OutErrors) const override;
	virtual FText GetProgressText() const override;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Objective")
	TArray<TObjectPtr<UOutbreakObjective>> SubObjectives;

	/** Sub-objectives that must complete. 0 = all of them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "0"))
	int32 RequiredCount = 1;

	/** RequiredCount resolved against the number of sub-objectives. */
	int32 GetNeededCount() const;

	int32 GetCompletedCount() const;
};
