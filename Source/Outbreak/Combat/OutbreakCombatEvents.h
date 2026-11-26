#pragma once

#include "CoreMinimal.h"

class AActor;
class AController;

/** Victim, the controller that caused the damage (may be null), damage applied, whether it killed or downed the victim. */
DECLARE_MULTICAST_DELEGATE_FourParams(FOutbreakDamageDealtEvent, AActor* /*Victim*/, AController* /*Instigator*/, float /*Damage*/, bool /*bTakenDown*/);

/**
 * Game-wide combat notifications (Phase 13), so presentation such as hit markers can react to
 * damage without the health system knowing about players or the HUD. Broadcast on the authority.
 */
namespace OutbreakCombatEvents
{
	OUTBREAK_API FOutbreakDamageDealtEvent& OnDamageDealt();
}
