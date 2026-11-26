#pragma once

#include "CoreMinimal.h"
#include "Missions/OutbreakObjective.h"

#include "OutbreakEliminateTargetObjective.generated.h"

/**
 * Completes when every actor tagged TargetTag (at activation) is dead or destroyed.
 * Tag enemies (or a single boss) in the level with the actor tag.
 */
UCLASS(DisplayName = "Eliminate Target")
class OUTBREAK_API UOutbreakEliminateTargetObjective : public UOutbreakObjective
{
	GENERATED_BODY()

public:

	virtual void Activate(UOutbreakMissionSubsystem& Mission) override;
	virtual EOutbreakObjectiveState Evaluate(UOutbreakMissionSubsystem& Mission) override;
	virtual bool Validate(TArray<FText>& OutErrors) const override;
	virtual FText GetProgressText() const override;

	/** Actor tag identifying the targets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName TargetTag;

	int32 GetTargetCount() const { return Targets.Num(); }
	int32 GetEliminatedCount() const { return EliminatedCount; }

private:

	static bool IsEliminated(const TWeakObjectPtr<AActor>& Target);

	TArray<TWeakObjectPtr<AActor>> Targets;
	int32 EliminatedCount = 0;
};
