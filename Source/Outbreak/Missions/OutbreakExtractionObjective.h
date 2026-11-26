#pragma once

#include "CoreMinimal.h"
#include "Missions/OutbreakObjective.h"

#include "OutbreakExtractionObjective.generated.h"

class AOutbreakMissionMarker;

/**
 * Extract from the area: every living player (and, with bRequireTeammates, every standing
 * teammate) must stay within Radius of the marker for ExtractionTime seconds. Leaving resets the
 * countdown. Downed and dead soldiers cannot extract and are reported as left behind.
 *
 * Usually the last objective, activating after the previous required one. When it completes the
 * soldiers in the zone are recorded as extracted for the mission results.
 */
UCLASS(DisplayName = "Extraction")
class OUTBREAK_API UOutbreakExtractionObjective : public UOutbreakObjective
{
	GENERATED_BODY()

public:

	virtual void Activate(UOutbreakMissionSubsystem& Mission) override;
	virtual EOutbreakObjectiveState Evaluate(UOutbreakMissionSubsystem& Mission) override;
	virtual void Deactivate() override;
	virtual bool Validate(TArray<FText>& OutErrors) const override;
	virtual FText GetProgressText() const override;

	/** AOutbreakMissionMarker::MarkerId of the extraction point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName MarkerId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "50.0", Units = "cm"))
	float Radius = 600.0f;

	/** Seconds everyone must stay in the zone. 0 = extract on arrival. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "0.0", Units = "s"))
	float ExtractionTime = 5.0f;

	/** Standing teammates must be in the zone too. Off = the players alone decide. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	bool bRequireTeammates = true;

private:

	TWeakObjectPtr<AOutbreakMissionMarker> Marker;
	float LastEvaluationTime = 0.0f;
	float HeldTime = 0.0f;
	bool bWasEveryoneIn = false;
	int32 InZoneCount = 0;
	int32 RequiredCount = 0;
	float ClosestPlayerDistance = -1.0f;
};
