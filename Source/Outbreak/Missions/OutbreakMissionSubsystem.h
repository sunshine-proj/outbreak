#pragma once

#include "CoreMinimal.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Missions/OutbreakMissionTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "OutbreakMissionSubsystem.generated.h"

class APawn;
class UOutbreakMissionDefinition;
class UOutbreakObjective;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakMissionStateChangedSignature, EOutbreakMissionState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakObjectiveChangedSignature, int32, ObjectiveIndex);

/**
 * Runs the current mission (server only): instantiates its objectives, activates them in order,
 * evaluates the current one at the mission's update interval, and decides completion and failure.
 *
 * Players are registered by the game mode when they spawn; the mission fails when all of them
 * are dead (if the definition says so). The status is published to AOutbreakGameState so every
 * client's HUD can show it.
 */
UCLASS()
class OUTBREAK_API UOutbreakMissionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Deinitialize() override;

	/**
	 * Prepares Definition: validates it and creates per-play copies of its objectives.
	 * Returns false (and logs why) if the definition is invalid; the mission then does not start.
	 */
	bool InitializeMission(const UOutbreakMissionDefinition* Definition);

	/** Activates the first objective and starts evaluation. Requires a successful InitializeMission. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void StartMission();

	/** Evaluates the current objective now. Runs automatically on a timer while in progress. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void UpdateMission();

	UFUNCTION(BlueprintCallable, Category = "Mission")
	void FailMission(const FText& Reason);

	/** Tracks a player pawn for death-based failure and for objectives such as Reach Location. */
	void RegisterPlayer(APawn* PlayerPawn);

	/** Registered player pawns that are alive. */
	TArray<APawn*> GetLivingPlayerPawns() const;

	UFUNCTION(BlueprintPure, Category = "Mission")
	EOutbreakMissionState GetMissionState() const { return MissionState; }

	UFUNCTION(BlueprintPure, Category = "Mission")
	int32 GetCurrentObjectiveIndex() const { return CurrentObjectiveIndex; }

	UFUNCTION(BlueprintPure, Category = "Mission")
	UOutbreakObjective* GetCurrentObjective() const;

	UFUNCTION(BlueprintPure, Category = "Mission")
	int32 GetObjectiveCount() const { return Objectives.Num(); }

	const UOutbreakMissionDefinition* GetMissionDefinition() const { return Definition; }

	/** The status published to clients. */
	FOutbreakMissionStatus BuildStatus() const;

	UPROPERTY(BlueprintAssignable, Category = "Mission")
	FOutbreakMissionStateChangedSignature OnMissionStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Mission")
	FOutbreakObjectiveChangedSignature OnObjectiveChanged;

private:

	void ActivateObjective(int32 Index);
	void CompleteMission();
	void EndMission(EOutbreakMissionState EndState, const FText& Result);
	void SetMissionState(EOutbreakMissionState NewState);
	void PublishStatus() const;
	void RestartLevel();

	UFUNCTION()
	void HandlePlayerHealthStateChanged(UOutbreakHealthComponent* HealthComponent, EOutbreakHealthState NewState, EOutbreakHealthState OldState);

	/** Fails the mission when no registered player stands and no squadmate can revive a downed one (Phase 10). */
	void CheckSquadFailure();

	/** Read-only at runtime; non-const only to be a UPROPERTY. */
	UPROPERTY(Transient)
	TObjectPtr<UOutbreakMissionDefinition> Definition;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UOutbreakObjective>> Objectives;

	TArray<TWeakObjectPtr<APawn>> Players;

	EOutbreakMissionState MissionState = EOutbreakMissionState::NotStarted;
	int32 CurrentObjectiveIndex = INDEX_NONE;
	FText ResultText;

	FTimerHandle UpdateTimer;
	FTimerHandle RestartTimer;
};
