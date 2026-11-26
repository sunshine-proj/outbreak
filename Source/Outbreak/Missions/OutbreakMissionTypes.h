#pragma once

#include "CoreMinimal.h"
#include "Squad/OutbreakSquadTypes.h"

#include "OutbreakMissionTypes.generated.h"

UENUM(BlueprintType)
enum class EOutbreakMissionState : uint8
{
	NotStarted,
	InProgress,
	Completed,
	Failed,
	/** Before deployment: players read the briefing and choose squad and equipment (Phase 11). */
	Briefing
};

UENUM(BlueprintType)
enum class EOutbreakObjectiveState : uint8
{
	Inactive,
	Active,
	Completed,
	Failed
};

/** When an objective becomes active (Phase 11 objective dependencies). */
UENUM(BlueprintType)
enum class EOutbreakObjectiveActivation : uint8
{
	/** After the nearest earlier required objective completes (at mission start if there is none). */
	AfterPrevious,
	/** As soon as the mission starts. */
	AtMissionStart,
	/** After every objective listed in Prerequisites completes. */
	AfterPrerequisites
};

/** One objective as clients see it. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakObjectiveStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FText Description;

	/** E.g. "2 / 5" or "35 m". May be empty. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FText Progress;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	EOutbreakObjectiveState State = EOutbreakObjectiveState::Inactive;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	bool bOptional = false;

	/** Not shown in the briefing or the HUD until it becomes active. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	bool bHidden = false;

	bool Identical(const FOutbreakObjectiveStatus& Other) const;
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

	/** Current objective's description: the first active required objective, else the first active optional one. */
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

	/** Every objective in definition order (Phase 11 full objective display). */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	TArray<FOutbreakObjectiveStatus> Objectives;

	bool Identical(const FOutbreakMissionStatus& Other) const;
};

/** One soldier in the mission results. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakMissionMemberReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	bool bIsPlayer = false;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	EOutbreakSquadMemberStatus Status = EOutbreakSquadMemberStatus::Active;

	/** Was inside the extraction zone when the squad extracted. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	bool bExtracted = false;
};

/**
 * The outcome of a finished mission (Phase 11 mission results). Built by the mission subsystem when
 * the mission ends, replicated through AOutbreakGameState, shown by the summary screen and meant
 * for the Phase 14 campaign.
 */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakMissionResults
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FName MissionId;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FText MissionName;

	/** Completed or Failed; NotStarted while the mission has not ended. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	EOutbreakMissionState Outcome = EOutbreakMissionState::NotStarted;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FText ResultText;

	/** Seconds from deployment to the end. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	float Duration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	TArray<FOutbreakObjectiveStatus> Objectives;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 RequiredObjectivesCompleted = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 RequiredObjectiveCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 OptionalObjectivesCompleted = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 OptionalObjectiveCount = 0;

	/** Every squad soldier, players first. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	TArray<FOutbreakMissionMemberReport> Squad;

	bool IsFinal() const { return Outcome == EOutbreakMissionState::Completed || Outcome == EOutbreakMissionState::Failed; }
};
