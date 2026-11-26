#pragma once

#include "CoreMinimal.h"
#include "Environment/OutbreakSwitchable.h"
#include "GameFramework/Actor.h"

#include "OutbreakAlarmSpeaker.generated.h"

class UAudioComponent;
class USoundBase;

/**
 * A siren for the base alarm (Phase 13): plays AlarmSound while the alarm is raised, on every
 * machine. Link it to equipment (an alarm panel) to let players silence the alarm: switching the
 * equipment on resets the alarm (IOutbreakSwitchable).
 */
UCLASS(Blueprintable)
class OUTBREAK_API AOutbreakAlarmSpeaker : public AActor, public IOutbreakSwitchable
{
	GENERATED_BODY()

public:

	AOutbreakAlarmSpeaker();

	UFUNCTION(BlueprintPure, Category = "Alarm")
	bool IsSounding() const;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//~ Begin IOutbreakSwitchable interface
	virtual void SetSwitchedOn_Implementation(bool bOn, AActor* SwitchedBy) override;
	//~ End IOutbreakSwitchable interface

	/** Blueprint feedback (rotating lights) on every machine. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Alarm")
	void OnAlarmStateChanged(bool bAlarmRaised);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudioComponent> Siren;

	/** A looping siren. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alarm")
	TObjectPtr<USoundBase> AlarmSound;

private:

	UFUNCTION()
	void HandleAlarmChanged(bool bAlarmRaised);
};
