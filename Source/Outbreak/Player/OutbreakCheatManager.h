#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"

#include "OutbreakCheatManager.generated.h"

/**
 * Profiling and QA console commands (Phase 15). Available in non-shipping builds, standalone or
 * with cheats enabled; they run on the server. Combine with `stat Outbreak`, `stat unit`, Unreal
 * Insights or `csvprofile start/stop` (Docs/Phase15-EditorSetup.md). For the maximum squad,
 * start the level with `?SquadSize=6?SkipBriefing`.
 */
UCLASS()
class OUTBREAK_API UOutbreakCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:

	/**
	 * Spawns Count enemies in a ring of Radius around the player. DefinitionPath: an enemy
	 * definition asset path (empty = class default). ClassPath: an enemy Blueprint class path
	 * (empty = AOutbreakEnemyCharacter). Example:
	 * `OutbreakSpawnEnemies 20 /Game/AI/Enemies/DA_Enemy_AlienInfantry.DA_Enemy_AlienInfantry`
	 */
	UFUNCTION(Exec)
	void OutbreakSpawnEnemies(int32 Count = 10, const FString& DefinitionPath = TEXT(""), const FString& ClassPath = TEXT(""), float Radius = 2500.0f);

	/** Alerts every living enemy to the player's position, so all of them enter combat at once (worst case for AI cost). */
	UFUNCTION(Exec)
	void OutbreakAlertEnemies();

	/** Kills every living enemy. */
	UFUNCTION(Exec)
	void OutbreakKillEnemies();

	/** Logs counts of enemies (by state), teammates and players, for profiling notes. */
	UFUNCTION(Exec)
	void OutbreakPerfReport();
};
