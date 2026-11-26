#pragma once

#include "CoreMinimal.h"

/**
 * Blackboard key names written by the Outbreak AI controllers. The Blackboard assets must define
 * keys with these names and types (Docs/Phase4-EditorSetup.md, Docs/Phase5-EditorSetup.md).
 */
namespace OutbreakBlackboardKeys
{
	/** Enum (EOutbreakTeammateBehavior). For debugging and decorators. */
	inline const FName Behavior(TEXT("Behavior"));

	/** Vector. Where the teammate should be. The Behavior Tree moves here. */
	inline const FName MoveLocation(TEXT("MoveLocation"));

	/** Object (Actor). Current combat target, if any. */
	inline const FName TargetActor(TEXT("TargetActor"));

	/** Object (Actor). The squad's player pawn (teammates). */
	inline const FName PlayerActor(TEXT("PlayerActor"));

	/** Enum (EOutbreakEnemyState). Enemy blackboard only. */
	inline const FName EnemyState(TEXT("State"));

	/** Enum (EOutbreakEnemyCombatMove). Enemy blackboard only, optional (Phase 9). */
	inline const FName CombatMove(TEXT("CombatMove"));
}
