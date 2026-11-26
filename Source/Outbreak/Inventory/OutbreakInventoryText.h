#pragma once

#include "CoreMinimal.h"

class AActor;
class UOutbreakInventoryComponent;

/** HUD text for the inventory and medical state (Phase 10 inventory UI), kept testable. */
namespace OutbreakInventoryText
{
	/**
	 * Compact: "Medkit x2  |  [Frag Grenade x3]  |  18.4 / 25 kg" (the selected utility in brackets).
	 * Expanded (inventory panel open): slots, ammunition, items and weight on separate lines.
	 */
	OUTBREAK_API FText BuildInventoryText(const UOutbreakInventoryComponent* Inventory, bool bExpanded);

	/**
	 * The owner's medical situation, empty when there is nothing to report:
	 * "DOWN - bleeding out in 23 s", "Being revived by Hawk 40%", "Reviving Hawk 60%".
	 */
	OUTBREAK_API FText BuildMedicalStatusText(const AActor* Soldier);
}
