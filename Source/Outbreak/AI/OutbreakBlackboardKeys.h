#pragma once

#include "CoreMinimal.h"

/**
 * Blackboard key names written by AOutbreakSquadAIController. The teammate Blackboard asset must
 * define keys with these names and types (Docs/Phase4-EditorSetup.md).
 */
namespace OutbreakBlackboardKeys
{
	/** Enum (EOutbreakTeammateBehavior). For debugging and decorators. */
	inline const FName Behavior(TEXT("Behavior"));

	/** Vector. Where the teammate should be. The Behavior Tree moves here. */
	inline const FName MoveLocation(TEXT("MoveLocation"));

	/** Object (Actor). Current combat target, if any. */
	inline const FName TargetActor(TEXT("TargetActor"));

	/** Object (Actor). The squad's player pawn. */
	inline const FName PlayerActor(TEXT("PlayerActor"));
}
