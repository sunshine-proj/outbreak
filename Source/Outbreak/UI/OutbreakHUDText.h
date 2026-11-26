#pragma once

#include "CoreMinimal.h"

/** HUD wording that is worth testing (Phase 13 HUD polish). */
namespace OutbreakHUDText
{
	/**
	 * Stealth awareness line: empty while unnoticed, a meter while an enemy is noticing you
	 * ("NOTICED [###-------]"), "DETECTED" at full detection, plus "ALARM RAISED" while the alarm is up.
	 * Awareness is the highest enemy detection of the player, 0..1.
	 */
	OUTBREAK_API FText BuildAwarenessText(float Awareness, bool bAlarmRaised);
}
