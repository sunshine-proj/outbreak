#pragma once

#include "CoreMinimal.h"

#include "OutbreakMissionTypes.generated.h"

UENUM(BlueprintType)
enum class EOutbreakMissionState : uint8
{
	NotStarted,
	InProgress,
	Completed,
	Failed
};

UENUM(BlueprintType)
enum class EOutbreakObjectiveState : uint8
{
	Inactive,
	Active,
	Completed,
	Failed
};

/**
 * What clients need to show the mission: replicated through AOutbreakGameState, because the
 * mission subsystem runs on the server only.
 */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakMissionStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	EOutbreakMissionState State = EOutbreakMissionState::NotStarted;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FText MissionName;

	/** Current objective's description. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FText ObjectiveDescription;

	/** Current objective's progress, e.g. "2 / 5" or "35 m". May be empty. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FText ObjectiveProgress;

	/** 0-based index of the current objective. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 ObjectiveIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 ObjectiveCount = 0;

	/** Why the mission failed, or the completion message. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FText ResultText;
};
