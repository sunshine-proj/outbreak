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
 * The subsystem activates an objective when its dependencies are met (Activation, Prerequisites)
 * and evaluates every active objective at its update interval, so several can run at once.
 * The mission completes when every required (non-optional) objective has completed and fails when
 * a required one fails. New objective types subclass this.
 *
 * Objectives refer to level actors by marker ID or actor tag, never by direct reference, because
 * a data asset cannot point into a level.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class OUTBREAK_API UOutbreakObjective : public UObject
{
	GENERATED_BODY()

public:

	/** Called once when the objective becomes active. Find level actors here. */
	virtual void Activate(UOutbreakMissionSubsystem& Mission) {}

	/** Returns Active, Completed or Failed. Called at the mission update interval while active. */
	virtual EOutbreakObjectiveState Evaluate(UOutbreakMissionSubsystem& Mission) PURE_VIRTUAL(UOutbreakObjective::Evaluate, return EOutbreakObjectiveState::Active;);

	/** Called when the objective stops being active (completed, failed, or the mission ended). Release level actors here. */
	virtual void Deactivate() {}

	/** Configuration problems, for data validation. */
	virtual bool Validate(TArray<FText>& OutErrors) const { return true; }

	/**
	 * Problems with references to other objectives of the same mission (Phase 14), e.g. an Avoid
	 * Alarm objective naming an unknown objective. ObjectiveIds holds every ID in the mission.
	 */
	virtual bool ValidateReferences(const TSet<FName>& ObjectiveIds, TArray<FText>& OutErrors) const { return true; }

	UFUNCTION(BlueprintPure, Category = "Objective")
	virtual FText GetDescription() const { return Description; }

	/** Progress shown next to the description ("2 / 5", "35 m"). Empty = none. */
	UFUNCTION(BlueprintPure, Category = "Objective")
	virtual FText GetProgressText() const { return FText::GetEmpty(); }

	UFUNCTION(BlueprintPure, Category = "Objective")
	EOutbreakObjectiveState GetState() const { return State; }

	void SetState(EOutbreakObjectiveState NewState) { State = NewState; }

	UFUNCTION(BlueprintPure, Category = "Objective")
	FName GetObjectiveId() const { return ObjectiveId; }

	UFUNCTION(BlueprintPure, Category = "Objective")
	bool IsOptional() const { return bOptional; }

	UFUNCTION(BlueprintPure, Category = "Objective")
	bool IsHidden() const { return bHidden; }

	/** Mission time when the objective last became active (set by the mission subsystem). */
	void SetActivationTime(float MissionTime) { ActivationTime = MissionTime; }

	/** Seconds left before the time limit fails the objective; negative when it has no time limit. */
	float GetTimeRemaining(float MissionTime) const { return TimeLimit > 0.0f ? FMath::Max(0.0f, ActivationTime + TimeLimit - MissionTime) : -1.0f; }

	/** The time limit has run out at MissionTime. */
	bool HasTimedOut(float MissionTime) const { return TimeLimit > 0.0f && MissionTime >= ActivationTime + TimeLimit; }

	EOutbreakObjectiveActivation GetActivation() const { return Activation; }
	const TArray<FName>& GetPrerequisites() const { return Prerequisites; }

	/** Shown on the HUD, e.g. "Reach the extraction point". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FText Description;

	/** Name other objectives use in Prerequisites. Optional; must be unique within the mission. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dependencies")
	FName ObjectiveId;

	/** When the objective becomes active. The default keeps objectives in list order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dependencies")
	EOutbreakObjectiveActivation Activation = EOutbreakObjectiveActivation::AfterPrevious;

	/** ObjectiveIds that must complete first. A required objective can only depend on required ones. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dependencies",
		meta = (EditCondition = "Activation == EOutbreakObjectiveActivation::AfterPrerequisites", EditConditionHides))
	TArray<FName> Prerequisites;

	/** Not needed to complete the mission; failing it does not fail the mission. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	bool bOptional = false;

	/** Kept out of the briefing and the HUD until it becomes active (a surprise objective). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	bool bHidden = false;

	/**
	 * Seconds to complete the objective once it is active (Phase 14); it fails when they run out.
	 * The HUD shows the time left. 0 = no time limit.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "0.0", Units = "s"))
	float TimeLimit = 0.0f;

	/**
	 * Where the squad resumes when a checkpoint saved on completing this objective is loaded
	 * (AOutbreakMissionMarker::MarkerId). Empty = the mission's deployment marker.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	FName CheckpointMarkerId;

private:

	EOutbreakObjectiveState State = EOutbreakObjectiveState::Inactive;
	float ActivationTime = 0.0f;
};
