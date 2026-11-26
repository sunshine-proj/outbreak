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

	/**
	 * Outbreak attitude rules: the same team is friendly, a missing team (Neutral) is neutral,
	 * different teams are hostile. Registered as the engine attitude solver by AOutbreakGameMode.
	 */
	inline ETeamAttitude::Type GetAttitude(FGenericTeamId A, FGenericTeamId B)
	{
		if (A == FGenericTeamId::NoTeam || B == FGenericTeamId::NoTeam)
		{
			return ETeamAttitude::Neutral;
		}
		return A == B ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
	}

	/** True when A and B are on hostile teams. Does not depend on the registered solver. */
	inline bool IsHostile(const AActor* A, const AActor* B)
	{
		return A && B
			&& GetAttitude(FGenericTeamId::GetTeamIdentifier(A), FGenericTeamId::GetTeamIdentifier(B)) == ETeamAttitude::Hostile;
	}
}
