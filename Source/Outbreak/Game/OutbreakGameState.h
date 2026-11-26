#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Missions/OutbreakMissionTypes.h"

#include "OutbreakGameState.generated.h"

class UOutbreakMissionDefinition;
class UOutbreakSquadConfiguration;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakMissionStatusChangedSignature, const FOutbreakMissionStatus&, Status);

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
};
