#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Missions/OutbreakMissionTypes.h"

#include "OutbreakGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakMissionStatusChangedSignature, const FOutbreakMissionStatus&, Status);

/** Replicates the mission status (set by UOutbreakMissionSubsystem on the server) to every client. */
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

	/** Fires on every machine when the status changes. */
	UPROPERTY(BlueprintAssignable, Category = "Mission")
	FOutbreakMissionStatusChangedSignature OnMissionStatusChanged;

private:

	UFUNCTION()
	void OnRep_MissionStatus();

	UPROPERTY(ReplicatedUsing = OnRep_MissionStatus)
	FOutbreakMissionStatus MissionStatus;
};
