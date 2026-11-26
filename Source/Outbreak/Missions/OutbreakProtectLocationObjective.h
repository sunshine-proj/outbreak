#pragma once

#include "CoreMinimal.h"
#include "Missions/OutbreakObjective.h"

#include "OutbreakProtectLocationObjective.generated.h"

class AOutbreakMissionMarker;

/**
 * Hold a location for Duration seconds. Completes when the time is up.
 *
 * Fails when living hostiles stay inside Radius of the marker for MaxHostileTime seconds without a
 * break (0 = the first hostile inside fails it), or when any actor tagged ProtectedTag is destroyed.
 * Hostile = on a team hostile to the player team.
 */
UCLASS(DisplayName = "Protect Location")
class OUTBREAK_API UOutbreakProtectLocationObjective : public UOutbreakObjective
{
	GENERATED_BODY()

public:

	virtual void Activate(UOutbreakMissionSubsystem& Mission) override;
	virtual EOutbreakObjectiveState Evaluate(UOutbreakMissionSubsystem& Mission) override;
	virtual void Deactivate() override;
	virtual bool Validate(TArray<FText>& OutErrors) const override;
	virtual FText GetProgressText() const override;

	/** AOutbreakMissionMarker::MarkerId at the centre of the protected zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName MarkerId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "50.0", Units = "cm"))
	float Radius = 1000.0f;

	/** Seconds the zone must be held. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "1.0", Units = "s"))
	float Duration = 60.0f;

	/** Seconds hostiles may occupy the zone without a break before it is lost. 0 = any hostile inside loses it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "0.0", Units = "s"))
	float MaxHostileTime = 5.0f;

	/** Optional: actors (for example AOutbreakDestructibleObject) that must survive. None = no such actors. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName ProtectedTag;

	bool IsContested() const { return bContested; }
	float GetTimeRemaining() const { return TimeRemaining; }

private:

	TWeakObjectPtr<AOutbreakMissionMarker> Marker;
	TArray<TWeakObjectPtr<AActor>> ProtectedActors;
	float StartTime = 0.0f;
	float LastEvaluationTime = 0.0f;
	float HostileTime = 0.0f;
	float TimeRemaining = 0.0f;
	bool bContested = false;
};
