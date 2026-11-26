#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GameFramework/GameStateBase.h"
#include "Missions/OutbreakMissionTypes.h"

#include "OutbreakGameState.generated.h"

class UOutbreakMissionDefinition;
class UOutbreakSquadConfiguration;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakMissionStatusChangedSignature, const FOutbreakMissionStatus&, Status);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakAlarmChangedSignature, bool, bAlarmRaised);

/**
 * Replicates the mission to every client: the definition and squad configuration the briefing
 * shows, the live status (set by UOutbreakMissionSubsystem on the server) and, at the end, the results.
 */
UCLASS()
class OUTBREAK_API AOutbreakGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Mission")
	FOutbreakMissionStatus GetMissionStatus() const { return MissionStatus; }

	/** Authority. */
	void SetMissionStatus(const FOutbreakMissionStatus& NewStatus);

	UFUNCTION(BlueprintPure, Category = "Mission")
	const FOutbreakMissionResults& GetMissionResults() const { return MissionResults; }

	/** Authority. Set before the final status so clients see both together. */
	void SetMissionResults(const FOutbreakMissionResults& NewResults);

	/** Authority: the mission and the squad configuration players choose from in the briefing. */
	void SetMissionSetup(UOutbreakMissionDefinition* InMissionDefinition, UOutbreakSquadConfiguration* InSquadConfiguration);

	UFUNCTION(BlueprintPure, Category = "Mission")
	UOutbreakMissionDefinition* GetMissionDefinition() const { return MissionDefinition; }

	/** The configuration the squad is built from (the mission's, the game mode's, or null = project default). */
	UFUNCTION(BlueprintPure, Category = "Mission")
	UOutbreakSquadConfiguration* GetSquadConfiguration() const { return SquadConfiguration; }

	/** Authority: set by UOutbreakAlarmSubsystem (Phase 13). */
	void SetAlarm(bool bRaised, const FVector& Location);

	UFUNCTION(BlueprintPure, Category = "Alarm")
	bool IsAlarmRaised() const { return bAlarmRaised; }

	FVector GetAlarmLocation() const { return AlarmLocation; }

	/** Fires on every machine when the alarm goes up or is silenced. */
	UPROPERTY(BlueprintAssignable, Category = "Alarm")
	FOutbreakAlarmChangedSignature OnAlarmChanged;

	/** Fires on every machine when the status changes. */
	UPROPERTY(BlueprintAssignable, Category = "Mission")
	FOutbreakMissionStatusChangedSignature OnMissionStatusChanged;

private:

	UFUNCTION()
	void OnRep_MissionStatus();

	UPROPERTY(ReplicatedUsing = OnRep_MissionStatus)
	FOutbreakMissionStatus MissionStatus;

	UPROPERTY(Replicated)
	FOutbreakMissionResults MissionResults;

	UPROPERTY(Replicated)
	TObjectPtr<UOutbreakMissionDefinition> MissionDefinition;

	UPROPERTY(Replicated)
	TObjectPtr<UOutbreakSquadConfiguration> SquadConfiguration;

	UFUNCTION()
	void OnRep_Alarm();

	UPROPERTY(ReplicatedUsing = OnRep_Alarm)
	bool bAlarmRaised = false;

	UPROPERTY(Replicated)
	FVector_NetQuantize AlarmLocation = FVector::ZeroVector;
};
