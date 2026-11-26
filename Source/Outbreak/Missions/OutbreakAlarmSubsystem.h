#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "OutbreakAlarmSubsystem.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakAlarmRaisedSignature, FVector, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOutbreakAlarmResetSignature);

/**
 * The base alarm (Phase 13, developer decision: configurable alarm on detection). Server only.
 *
 * When an enemy enters Combat, the alarm is pending for the mission's Alarm Delay; if that spotter
 * is still alive and fighting when the delay ends, the alarm is raised. Silencing the spotter in
 * time keeps the alarm down. Encounters with the Alarm trigger bring reinforcements, alarm speakers
 * sound the siren, and the squad calls it out. Free play (no mission) or a mission with the alarm
 * disabled never raises it. Replicated to clients through AOutbreakGameState.
 */
UCLASS()
class OUTBREAK_API UOutbreakAlarmSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Deinitialize() override;

	/** An enemy has detected the squad. Starts the alarm countdown unless one is pending or raised. */
	void ReportCombatStarted(AActor* Spotter);

	/** Raises the alarm now (scripts, alarm panels). Ignored if already raised. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Alarm")
	void RaiseAlarm(FVector Location, AActor* RaisedBy);

	/** Silences the alarm (an alarm panel). Encounters already triggered stay triggered. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Alarm")
	void ResetAlarm();

	UFUNCTION(BlueprintPure, Category = "Alarm")
	bool IsAlarmRaised() const { return bRaised; }

	UFUNCTION(BlueprintPure, Category = "Alarm")
	bool IsAlarmPending() const { return PendingSpotter.IsValid(); }

	/** The current mission enables the alarm. */
	bool IsAlarmEnabled() const;

	/** Ends the countdown: raises the alarm if the spotter can still raise it. Called by the countdown timer. */
	void ResolvePendingAlarm();

	UPROPERTY(BlueprintAssignable, Category = "Alarm")
	FOutbreakAlarmRaisedSignature OnAlarmRaised;

	UPROPERTY(BlueprintAssignable, Category = "Alarm")
	FOutbreakAlarmResetSignature OnAlarmReset;

private:

	static bool CanStillRaise(const AActor* Spotter);
	void PublishAlarmState() const;

	TWeakObjectPtr<AActor> PendingSpotter;
	FTimerHandle PendingTimer;
	FVector AlarmLocation = FVector::ZeroVector;
	bool bRaised = false;
};
