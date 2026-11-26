#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"

#include "OutbreakTeam.generated.h"

/**
 * Team affiliation, used by AI perception (through IGenericTeamAgentInterface) to tell friend
 * from foe. Values map directly to FGenericTeamId.
 */
UENUM(BlueprintType)
enum class EOutbreakTeam : uint8
{
	Player = 1,
	Hostile = 2,
	Neutral = 255 UMETA(DisplayName = "Neutral (no team)")
};

namespace OutbreakTeam
{
	inline FGenericTeamId ToGenericTeamId(EOutbreakTeam Team)
	{
		return FGenericTeamId(static_cast<uint8>(Team));
	}
}
