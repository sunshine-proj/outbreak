#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "OutbreakAIAgent.generated.h"

class AActor;
class UOutbreakWeaponComponent;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UOutbreakAIAgent : public UInterface
{
	GENERATED_BODY()
};

/**
 * What the shared Behavior Tree services need from an Outbreak AI controller. Implemented by the
 * squad (teammate) and enemy controllers, so one decision service and one combat service drive both.
 */
class OUTBREAK_API IOutbreakAIAgent
{
	GENERATED_BODY()

public:

	/** Re-evaluate perception, state and destination, and update the blackboard. */
	virtual void UpdateBehavior() = 0;

	/** The actor to shoot at, if any. */
	virtual AActor* GetCombatTarget() const = 0;

	/** Whether Actor is currently seen. */
	virtual bool IsActorVisible(const AActor* Actor) const = 0;

	/** Maximum firing distance. */
	virtual float GetEngagementRange() const = 0;

	/** Fire only when the view is within this angle of the target, in degrees. */
	virtual float GetAimTolerance() const = 0;

	/** With no target, reload below this magazine fraction. */
	virtual float GetIdleReloadFraction() const = 0;

	virtual UOutbreakWeaponComponent* GetAgentWeaponComponent() const = 0;

	/** False while the agent must hold fire (squad stealth). Aiming and reloading continue. */
	virtual bool IsFireAllowed() const { return true; }

	/** Where the threat to take cover from is (EQS context "Outbreak: Threat"). False if none is known. */
	virtual bool GetCoverThreatLocation(FVector& OutLocation) const { return false; }

	/** Where cover should be searched around (EQS context "Outbreak: Cover Anchor"). False = the agent itself. */
	virtual bool GetCoverAnchorLocation(FVector& OutLocation) const { return false; }
};
