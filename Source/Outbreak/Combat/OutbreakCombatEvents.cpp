#include "Combat/OutbreakCombatEvents.h"

namespace OutbreakCombatEvents
{
	FOutbreakDamageDealtEvent& OnDamageDealt()
	{
		static FOutbreakDamageDealtEvent Event;
		return Event;
	}
}
