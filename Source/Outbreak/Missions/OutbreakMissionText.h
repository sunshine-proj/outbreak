#pragma once

#include "CoreMinimal.h"
#include "Missions/OutbreakMissionTypes.h"

class UOutbreakMissionDefinition;
class UOutbreakSquadConfiguration;
struct FOutbreakDeploymentSelection;

/**
 * Text for the mission UI (Phase 11): the objective display, objective notifications, the briefing
 * and the summary. Pure functions, so the default C++ widgets and Widget Blueprints show the same
 * thing and the wording is testable.
 */
namespace OutbreakMissionText
{
	/**
	 * The HUD objective list. Compact: active objectives only, required first, with progress.
	 * Expanded (the Tab panel): every revealed objective with its state. Hidden objectives appear once active.
	 */
	OUTBREAK_API FText BuildObjectiveList(const FOutbreakMissionStatus& Status, bool bExpanded);

	/** Notifications for objective changes between two statuses: new, completed and failed objectives. */
	OUTBREAK_API TArray<FText> BuildObjectiveNotifications(const FOutbreakMissionStatus& Previous, const FOutbreakMissionStatus& Current);

	/** Description, briefing and the revealed objectives of a mission. */
	OUTBREAK_API FText BuildBriefing(const UOutbreakMissionDefinition* Mission);

	/** Name for teammate slot Slot of a selection: the roster soldier's name, or the filler's ("Reinforcement" when unnamed). */
	OUTBREAK_API FText GetTeammateName(const UOutbreakSquadConfiguration* Configuration, const FOutbreakDeploymentSelection& Selection, int32 Slot);

	/** Loadout name for member MemberIndex (0 = player), "Default equipment" when none is chosen. */
	OUTBREAK_API FText GetLoadoutName(const FOutbreakDeploymentSelection& Selection, int32 MemberIndex);

	/** "MISSION COMPLETE" or "MISSION FAILED". */
	OUTBREAK_API FText BuildResultTitle(const FOutbreakMissionResults& Results);

	/** Result, time, objectives and squad report. */
	OUTBREAK_API FText BuildSummary(const FOutbreakMissionResults& Results);
}
