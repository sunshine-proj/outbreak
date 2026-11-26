#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Missions/OutbreakMissionTypes.h"

#include "OutbreakEncounter.generated.h"

class AOutbreakEnemyCharacter;
class AOutbreakPatrolRoute;
class UBoxComponent;
class UOutbreakEnemyDefinition;
class UPrimitiveComponent;

/** What starts an encounter (Phase 13). */
UENUM(BlueprintType)
enum class EOutbreakEncounterTrigger : uint8
{
	/** Only Trigger() (Blueprint or level script). */
	Manual,
	/** When the mission starts (after deployment). */
	MissionStart,
	/** When the objective ObjectiveId becomes active. */
	ObjectiveActivated,
	/** When the objective ObjectiveId completes. */
	ObjectiveCompleted,
	/** When a player walks into the trigger area. */
	PlayerEntersArea,
	/** When the base alarm is raised (developer decision: reinforcements on detection). */
	Alarm
};

/** What spawned enemies do first. */
UENUM(BlueprintType)
enum class EOutbreakEncounterOrder : uint8
{
	/** Patrol the encounter's route, or stand guard where they spawned. */
	Guard,
	/** Alerted towards the investigate target (or the encounter) and search it. */
	Investigate,
	/** Alerted towards the nearest player: a counterattack. */
	Hunt
};

/** Some enemies of one archetype. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakEncounterGroup
{
	GENERATED_BODY()

	/** Enemy Blueprint. Empty = the base enemy class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	TSubclassOf<AOutbreakEnemyCharacter> EnemyClass;

	/** Overrides the class's archetype. Empty = the class default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	TObjectPtr<UOutbreakEnemyDefinition> Definition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "1"))
	int32 Count = 2;
};

/**
 * A scripted enemy group (Phase 13 encounters): reinforcements arriving at an objective, a
 * counterattack during extraction, guards that appear when the alarm goes up.
 *
 * Spawns its Groups at the Spawn Points (round robin; the encounter itself when there are none)
 * after Delay, once its Trigger fires; spawned enemies carry SpawnedTag so Eliminate Target
 * objectives can count them, and follow Order. Fires once unless bRepeatable. Server only.
 */
UCLASS()
class OUTBREAK_API AOutbreakEncounter : public AActor
{
	GENERATED_BODY()

public:

	AOutbreakEncounter();

	/** Spawns the encounter (after Delay). Ignored once triggered unless bRepeatable. Authority. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Encounter")
	void Trigger();

	/** Subscribes to the mission, the alarm or the trigger area for Trigger. Called on BeginPlay. */
	void BindTrigger();

	UFUNCTION(BlueprintPure, Category = "Encounter")
	bool HasTriggered() const { return bTriggered; }

	/** Every spawned enemy is dead or gone (false before spawning). */
	UFUNCTION(BlueprintPure, Category = "Encounter")
	bool IsCleared() const;

	TArray<AOutbreakEnemyCharacter*> GetSpawnedEnemies() const;

	/** Configuration (normally set in the level). */
	void Configure(EOutbreakEncounterTrigger InTrigger, FName InObjectiveId, const TArray<FOutbreakEncounterGroup>& InGroups,
		const TArray<AActor*>& InSpawnPoints, FName InSpawnedTag, float InDelay = 0.0f,
		EOutbreakEncounterOrder InOrder = EOutbreakEncounterOrder::Hunt);

	/** Objective trigger: called by the mission subsystem's objective event. */
	UFUNCTION()
	void HandleObjectiveStateChanged(int32 ObjectiveIndex, EOutbreakObjectiveState NewState);

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Blueprint notification after spawning (music sting, radio message). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Encounter")
	void OnEncounterSpawned();

	/** For the PlayerEntersArea trigger. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter")
	TObjectPtr<UBoxComponent> TriggerArea;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	EOutbreakEncounterTrigger TriggerType = EOutbreakEncounterTrigger::Manual;

	/** For the objective triggers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	FName ObjectiveId;

	/** Seconds from the trigger to spawning (time for a radio warning, a dropship to arrive). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "0.0", Units = "s"))
	float Delay = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	TArray<FOutbreakEncounterGroup> Groups;

	/** Where enemies appear (Target Points or any actors). Empty = here. Keep them out of the players' sight. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter")
	TArray<TObjectPtr<AActor>> SpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	EOutbreakEncounterOrder Order = EOutbreakEncounterOrder::Hunt;

	/** Guard order: the route to patrol. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter")
	TObjectPtr<AOutbreakPatrolRoute> PatrolRoute;

	/** Investigate order: where to go. Empty = this encounter's location. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter")
	TObjectPtr<AActor> InvestigateTarget;

	/** Actor tag given to every spawned enemy (for Eliminate Target objectives). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	FName SpawnedTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	bool bRepeatable = false;

private:

	void Spawn();
	void ApplyOrder(AOutbreakEnemyCharacter* Enemy) const;
	FTransform GetSpawnTransform(int32 SpawnIndex) const;

	UFUNCTION()
	void HandleMissionStateChanged(EOutbreakMissionState NewState);

	UFUNCTION()
	void HandleAlarmRaised(FVector Location);

	UFUNCTION()
	void HandleAreaOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	TArray<TWeakObjectPtr<AOutbreakEnemyCharacter>> Spawned;
	FTimerHandle DelayTimer;
	bool bTriggered = false;
	bool bBound = false;
};
