#pragma once

#include "CoreMinimal.h"
#include "Missions/OutbreakMissionTypes.h"
#include "Squad/OutbreakSquadSnapshot.h"

#include "OutbreakMissionCheckpoint.generated.h"

struct FOutbreakObjectiveNode;

/**
 * A mission in progress, saved when a required objective completes (Phase 14 mission state
 * persistence, D-023). Loading rebuilds the level from scratch and resumes from here: completed and
 * failed objectives keep their state, the rest activate again from the start, and the squad is
 * rebuilt from the saved selection with each member's saved state.
 */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakMissionCheckpoint
{
	GENERATED_BODY()

	/** Campaign mission ID the checkpoint belongs to. None = no checkpoint. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	FName MissionId;

	/** One state per objective, in definition order. Active objectives are saved as Inactive. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	TArray<EOutbreakObjectiveState> ObjectiveStates;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	float MissionTime = 0.0f;

	/** Marker the squad resumes at (the completed objective's checkpoint marker, else the deployment marker). */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	FName ResumeMarkerId;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	bool bAlarmRaised = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	FVector AlarmLocation = FVector::ZeroVector;

	/** The host player's squad. Other co-op players deploy with the default squad. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	FOutbreakSquadSnapshot Squad;

	/** Objectives completed when saved (shown in the save slot list). */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Checkpoint")
	int32 CompletedObjectiveCount = 0;

	bool IsSet() const { return !MissionId.IsNone(); }
};

namespace OutbreakMissionCheckpointRules
{
	/**
	 * Whether Checkpoint can resume a mission whose objectives are Nodes: one state per objective, no
	 * required objective failed, required objectives left to do, and every completed objective's
	 * prerequisites completed. Appends a reason for each problem.
	 */
	OUTBREAK_API bool Validate(const FOutbreakMissionCheckpoint& Checkpoint, const TArray<FOutbreakObjectiveNode>& Nodes, TArray<FText>& OutErrors);

	/** Active objectives become Inactive (they restart); anything outside the enum becomes Inactive. */
	OUTBREAK_API EOutbreakObjectiveState ToSavedState(EOutbreakObjectiveState State);
}
