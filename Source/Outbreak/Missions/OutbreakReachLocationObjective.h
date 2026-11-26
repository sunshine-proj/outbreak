#pragma once

#include "CoreMinimal.h"
#include "Missions/OutbreakObjective.h"

#include "OutbreakReachLocationObjective.generated.h"

class AOutbreakMissionMarker;

/** Completes when a living player comes within Radius of the marker MarkerId. */
UCLASS(DisplayName = "Reach Location")
class OUTBREAK_API UOutbreakReachLocationObjective : public UOutbreakObjective
{
	GENERATED_BODY()

public:

	virtual void Activate(UOutbreakMissionSubsystem& Mission) override;
	virtual EOutbreakObjectiveState Evaluate(UOutbreakMissionSubsystem& Mission) override;
	virtual bool Validate(TArray<FText>& OutErrors) const override;
	virtual FText GetProgressText() const override;

	/** AOutbreakMissionMarker::MarkerId of the destination. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName MarkerId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "10.0", Units = "cm"))
	float Radius = 300.0f;

private:

	TWeakObjectPtr<AOutbreakMissionMarker> Marker;
	float LastDistance = -1.0f;
};
