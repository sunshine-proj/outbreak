#pragma once

#include "CoreMinimal.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Missions/OutbreakMissionTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "OutbreakMissionSubsystem.generated.h"

class AActor;
class APawn;
class UOutbreakMissionDefinition;
class UOutbreakObjective;
struct FOutbreakMissionCheckpoint;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakMissionStateChangedSignature, EOutbreakMissionState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakObjectiveChangedSignature, int32, ObjectiveIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOutbreakObjectiveStateChangedSignature, int32, ObjectiveIndex, EOutbreakObjectiveState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakCheckpointReachedSignature, int32, CompletedObjectiveIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakMissionEndedSignature, const FOutbreakMissionResults&, Results);

/**
 * Runs the current mission (server only).
 *
 * Flow: InitializeMission → (BeginBriefing → every registered player deploys) → StartMission →
 * objectives activate as their dependencies are met and every active one is evaluated at the
 * mission's update interval → Completed (all required objectives done) or Failed (a required
 * objective failed, or the squad is down) → results are built and the mission is cleaned up.
 *
 * Players are registered by the game mode when they spawn and marked deployed when they leave the
 * briefing. The status (and, at the end, the results) are published to AOutbreakGameState so every
 * client can show them.
 */
UCLASS()
class OUTBREAK_API UOutbreakMissionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Deinitialize() override;

	/**
	 * Prepares Definition: cleans up any previous mission, validates Definition and creates per-play
	 * copies of its objectives. Returns false (and logs why) if it is invalid; the mission then does not start.
	 */
	bool InitializeMission(const UOutbreakMissionDefinition* Definition);

	/** Enters the briefing (NotStarted → Briefing). The mission starts when every registered player has deployed. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void BeginBriefing();

	/** Activates the objectives that have no unmet dependencies and starts evaluation. From NotStarted or Briefing. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void StartMission();

	/**
	 * Starts from a checkpoint (Phase 14): completed and failed objectives keep their saved state
	 * without running again, mission time continues, and the rest activate by their dependencies.
	 * Falls back to StartMission (and returns false, logged) when the checkpoint does not fit this
	 * mission's objectives. The caller checks it belongs to this mission. From NotStarted or Briefing.
	 */
	bool StartMissionFromCheckpoint(const FOutbreakMissionCheckpoint& Checkpoint);

	/** Whether Checkpoint fits the initialized mission's objectives (OutbreakMissionCheckpointRules::Validate). */
	bool CanResumeFrom(const FOutbreakMissionCheckpoint& Checkpoint, TArray<FText>& OutErrors) const;

	/**
	 * The mission part of a checkpoint after CompletedObjectiveIndex completed: objective states,
	 * mission time and the resume marker. The caller adds the squad and the alarm.
	 */
	FOutbreakMissionCheckpoint CaptureCheckpoint(int32 CompletedObjectiveIndex) const;

	/** Evaluates the active objectives now without advancing mission time. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void UpdateMission();

	/** Advances mission time by DeltaSeconds, then evaluates. The update timer calls this with the update interval. */
	void AdvanceMission(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Mission")
	void FailMission(const FText& Reason);

	/** Replays the level. Only after the mission has ended (the summary screen's Retry). */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void RestartMission();

	// --- Players and deployment ---

	/** Tracks a player pawn for death-based failure and for objectives such as Reach Location. */
	void RegisterPlayer(APawn* PlayerPawn);

	/**
	 * Moves PlayerPawn to the mission's starting location (DeploymentMarkerId), offset sideways per
	 * deployed player. Returns false and leaves the pawn where it is when the mission has no marker
	 * or the marker is missing (logged).
	 */
	bool MovePawnToDeployment(APawn* PlayerPawn, FName MarkerIdOverride = NAME_None) const;

	/**
	 * PlayerPawn has left the briefing with its squad. Registers it; during the briefing, starts the
	 * mission once every registered player has deployed.
	 */
	void NotifyPlayerDeployed(APawn* PlayerPawn);

	bool IsPlayerDeployed(const APawn* PlayerPawn) const;
	bool AreAllPlayersDeployed() const;

	/** Registered player pawns that are alive. */
	TArray<APawn*> GetLivingPlayerPawns() const;

	/** Every registered player and their squads' members (living or not), players first, without duplicates. */
	TArray<AActor*> GetSquadSoldiers() const;

	/** Extraction objectives record who left the area; the results report it. */
	void RecordExtracted(const TArray<AActor*>& Soldiers);
	bool WasExtracted(const AActor* Soldier) const;

	// --- State ---

	UFUNCTION(BlueprintPure, Category = "Mission")
	EOutbreakMissionState GetMissionState() const { return MissionState; }

	/** The first active required objective, else the first active optional one, else the last one that was. */
	UFUNCTION(BlueprintPure, Category = "Mission")
	int32 GetCurrentObjectiveIndex() const { return CurrentObjectiveIndex; }

	UFUNCTION(BlueprintPure, Category = "Mission")
	UOutbreakObjective* GetCurrentObjective() const;

	UFUNCTION(BlueprintPure, Category = "Mission")
	UOutbreakObjective* GetObjective(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Mission")
	int32 GetObjectiveCount() const { return Objectives.Num(); }

	/** The objective with ObjectiveId, or null. */
	UFUNCTION(BlueprintPure, Category = "Mission")
	UOutbreakObjective* FindObjectiveById(FName ObjectiveId) const;

	/** Seconds of play since StartMission (advanced by the update timer). */
	UFUNCTION(BlueprintPure, Category = "Mission")
	float GetMissionTime() const { return MissionTime; }

	const UOutbreakMissionDefinition* GetMissionDefinition() const { return Definition; }

	/** The status published to clients. */
	FOutbreakMissionStatus BuildStatus() const;

	/** The current results; final once the mission has ended. */
	FOutbreakMissionResults BuildResults() const;

	/** Results captured when the mission ended (Outcome NotStarted before that). */
	const FOutbreakMissionResults& GetMissionResults() const { return FinalResults; }

	UPROPERTY(BlueprintAssignable, Category = "Mission")
	FOutbreakMissionStateChangedSignature OnMissionStateChanged;

	/** An objective became active (kept from Phase 6: fires with the activated objective's index). */
	UPROPERTY(BlueprintAssignable, Category = "Mission")
	FOutbreakObjectiveChangedSignature OnObjectiveChanged;

	/** An objective became Active, Completed or Failed. */
	UPROPERTY(BlueprintAssignable, Category = "Mission")
	FOutbreakObjectiveStateChangedSignature OnObjectiveStateChanged;

	/**
	 * A required objective completed and the mission goes on (Phase 14), after the objectives it
	 * unlocked have activated. Fires only when the definition enables checkpoints.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Mission")
	FOutbreakCheckpointReachedSignature OnCheckpointReached;

	/** The mission ended; Results are final. Fires before the final status is published. */
	UPROPERTY(BlueprintAssignable, Category = "Mission")
	FOutbreakMissionEndedSignature OnMissionEnded;

private:

	void ActivateObjective(int32 Index);
	/** Sets up evaluation once objectives are in place (StartMission and StartMissionFromCheckpoint). */
	void BeginEvaluation();
	void FinishObjective(int32 Index, EOutbreakObjectiveState EndState);
	/** Activates every inactive objective whose prerequisites completed; marks blocked ones failed. */
	void ActivateReadyObjectives();
	void UpdateCurrentObjectiveIndex();
	bool AreRequiredObjectivesComplete() const;
	TArray<EOutbreakObjectiveState> GetObjectiveStates() const;

	void CompleteMission();
	void EndMission(EOutbreakMissionState EndState, const FText& Result);
	/** Stops evaluation, deactivates active objectives and unbinds player events. Keeps the definition, states and results. */
	void CleanupMission();
	void SetMissionState(EOutbreakMissionState NewState);
	void PublishStatus() const;
	void HandleUpdateTimer();

	UFUNCTION()
	void HandlePlayerHealthStateChanged(UOutbreakHealthComponent* HealthComponent, EOutbreakHealthState NewState, EOutbreakHealthState OldState);

	/** Fails the mission when no registered player stands and no squadmate can revive a downed one (Phase 10). */
	void CheckSquadFailure();

	/** Read-only at runtime; non-const only to be a UPROPERTY. */
	UPROPERTY(Transient)
	TObjectPtr<UOutbreakMissionDefinition> Definition;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UOutbreakObjective>> Objectives;

	/** Resolved prerequisite indices per objective. */
	TArray<TArray<int32>> ObjectivePrerequisites;

	TArray<TWeakObjectPtr<APawn>> Players;
	TArray<TWeakObjectPtr<APawn>> DeployedPlayers;
	TArray<TWeakObjectPtr<AActor>> ExtractedSoldiers;

	EOutbreakMissionState MissionState = EOutbreakMissionState::NotStarted;
	int32 CurrentObjectiveIndex = INDEX_NONE;
	/** The last required objective completed during this update, for OnCheckpointReached. */
	int32 PendingCheckpointIndex = INDEX_NONE;
	float MissionTime = 0.0f;
	FText ResultText;
	FOutbreakMissionResults FinalResults;

	FTimerHandle UpdateTimer;
	FTimerHandle RestartTimer;
};
