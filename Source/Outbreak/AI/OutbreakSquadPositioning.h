#pragma once

#include "CoreMinimal.h"
#include "Squad/OutbreakSquadTypes.h"

/**
 * Tactical slot positions relative to a leader, for any number of slots (no fixed squad size).
 * Offsets are in the leader's local space: X forward, Y right. Every formation gives each slot a
 * distinct position at least about half the spacing from every other slot.
 */
namespace OutbreakSquadPositioning
{
	/** Offset of slot SlotIndex (0-based) of SlotCount in Formation. */
	OUTBREAK_API FVector ComputeFormationOffset(EOutbreakFormation Formation, int32 SlotIndex, int32 SlotCount, float Spacing);

	/** World location of a formation slot. Only the leader's yaw is used. */
	OUTBREAK_API FVector ComputeFormationLocation(const FVector& LeaderLocation, const FRotator& LeaderRotation,
		EOutbreakFormation Formation, int32 SlotIndex, int32 SlotCount, float Spacing);

	OUTBREAK_API FText GetFormationDisplayName(EOutbreakFormation Formation);
}
