#pragma once

#include "CoreMinimal.h"
#include "Missions/OutbreakMissionTypes.h"
#include "UObject/Object.h"

#include "OutbreakObjective.generated.h"

class UOutbreakMissionSubsystem;

/**
 * Base for mission objectives. Objectives are configured inline in a UOutbreakMissionDefinition
 * and duplicated per play by the mission subsystem, so runtime state never touches the asset.
 *
 * The subsystem activates objectives in order and evaluates the active one at its update
 * interval. New objective types (Destroy, Protect, Retrieve, Extraction in Phase 11) subclass this.
 * Objectives refer to level actors by marker ID or actor tag, never by direct reference, because
 * a data asset cannot point into a level.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class OUTBREAK_API UOutbreakObjective : public UObject
{
	GENERATED_BODY()

public:

	/** Called once when the objective becomes current. Find level actors here. */
	virtual void Activate(UOutbreakMissionSubsystem& Mission) {}

	/** Returns Active, Completed or Failed. Called at the mission update interval while current. */
	virtual EOutbreakObjectiveState Evaluate(UOutbreakMissionSubsystem& Mission) PURE_VIRTUAL(UOutbreakObjective::Evaluate, return EOutbreakObjectiveState::Active;);

	/** Called when the objective stops being current. */
	virtual void Deactivate() {}

	/** Configuration problems, for data validation. */
	virtual bool Validate(TArray<FText>& OutErrors) const { return true; }

	UFUNCTION(BlueprintPure, Category = "Objective")
	virtual FText GetDescription() const { return Description; }

	/** Progress shown next to the description ("2 / 5", "35 m"). Empty = none. */
	UFUNCTION(BlueprintPure, Category = "Objective")
	virtual FText GetProgressText() const { return FText::GetEmpty(); }

	UFUNCTION(BlueprintPure, Category = "Objective")
	EOutbreakObjectiveState GetState() const { return State; }

	void SetState(EOutbreakObjectiveState NewState) { State = NewState; }

protected:

	/** Shown on the HUD, e.g. "Reach the extraction point". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FText Description;

private:

	EOutbreakObjectiveState State = EOutbreakObjectiveState::Inactive;
};
