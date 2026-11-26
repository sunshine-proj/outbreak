#pragma once

#include "CoreMinimal.h"
#include "Squad/OutbreakSquadTypes.h"

#include "OutbreakSquadSelection.generated.h"

class UOutbreakLoadoutDefinition;
class UOutbreakSquadConfiguration;
struct FOutbreakSquadMemberConfig;

/**
 * The squad a player chose in the briefing (Phase 11 squad and equipment selection): how many
 * soldiers, which roster soldiers fill the teammate slots, and each member's loadout.
 *
 * The roster is the squad configuration's Teammates list. A slot set to INDEX_NONE takes the
 * configuration's FillerTeammate (a generic reinforcement). Plain data, so it travels in one RPC
 * and the server validates it.
 */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakDeploymentSelection
{
	GENERATED_BODY()

	/** Total soldiers, the player included. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	int32 SquadSize = 0;

	/** Roster index per teammate slot (SquadSize - 1 entries). INDEX_NONE = filler. Each roster soldier at most once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	TArray<int32> TeammateRosterIndices;

	/**
	 * Loadout per member: [0] the player, [1..] the teammate slots. Null (or missing) = the default
	 * equipment. Non-null entries must be in the configuration's AvailableLoadouts.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	TArray<TObjectPtr<UOutbreakLoadoutDefinition>> MemberLoadouts;
};

/** Squad selection rules. Configuration may be null (project default size, generic teammates, no loadouts). */
namespace OutbreakSquadSelection
{
	/** The configuration's limits under the project limits, or the project limits when it narrows them invalidly. */
	OUTBREAK_API FOutbreakSquadSizeLimits ResolveLimits(const UOutbreakSquadConfiguration* Configuration);

	/**
	 * The squad InitializeSquad would build: RequestedSize (0 = the configuration's size, else the
	 * default size, clamped into Limits), roster soldiers in order, then fillers, default equipment.
	 */
	OUTBREAK_API FOutbreakDeploymentSelection MakeDefault(const UOutbreakSquadConfiguration* Configuration,
		const FOutbreakSquadSizeLimits& Limits, int32 RequestedSize = 0);

	/** Returns false, appending a reason for each problem: size, slot count, roster indices, duplicates, loadouts. */
	OUTBREAK_API bool Validate(const UOutbreakSquadConfiguration* Configuration, const FOutbreakSquadSizeLimits& Limits,
		const FOutbreakDeploymentSelection& Selection, TArray<FText>& OutErrors);

	/** Resizes to NewSize (clamped into Limits): new slots take the next unused roster soldier, else a filler. */
	OUTBREAK_API void SetSquadSize(const UOutbreakSquadConfiguration* Configuration, const FOutbreakSquadSizeLimits& Limits,
		FOutbreakDeploymentSelection& Selection, int32 NewSize);

	/** Steps teammate slot Slot through the roster soldiers not used by other slots, and the filler. Direction ±1. */
	OUTBREAK_API void CycleTeammate(const UOutbreakSquadConfiguration* Configuration, FOutbreakDeploymentSelection& Selection,
		int32 Slot, int32 Direction);

	/** Steps member MemberIndex (0 = player) through default equipment and the AvailableLoadouts. Direction ±1. */
	OUTBREAK_API void CycleLoadout(const UOutbreakSquadConfiguration* Configuration, FOutbreakDeploymentSelection& Selection,
		int32 MemberIndex, int32 Direction);

	/** The member configuration for teammate slot Slot (roster entry or filler); an empty one without a configuration. */
	OUTBREAK_API const FOutbreakSquadMemberConfig& GetTeammateConfig(const UOutbreakSquadConfiguration* Configuration,
		const FOutbreakDeploymentSelection& Selection, int32 Slot);

	/** Member MemberIndex's chosen loadout, or null for default equipment. */
	OUTBREAK_API UOutbreakLoadoutDefinition* GetLoadout(const FOutbreakDeploymentSelection& Selection, int32 MemberIndex);
}
