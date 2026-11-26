#pragma once

#include "CoreMinimal.h"

/**
 * Tactical positions for teammates relative to the player. Phase 4 has one simple arrangement:
 * rows behind the player, each slot distinct so teammates don't converge. Named formations
 * (Column, Wedge, Line, ...) replace it in Phase 7.
 */
namespace OutbreakSquadPositioning
{
	/**
	 * Offset of follow slot SlotIndex (0-based) out of SlotCount, in the leader's local space
	 * (X forward, Y right). Slots fill rows of RowWidth behind the leader, centred on its line.
	 */
	OUTBREAK_API FVector ComputeFollowOffset(int32 SlotIndex, int32 SlotCount, float FollowDistance, float Spacing, int32 RowWidth);

	/** World location of a follow slot. Only the leader's yaw is used. */
	OUTBREAK_API FVector ComputeFollowLocation(const FVector& LeaderLocation, const FRotator& LeaderRotation,
		int32 SlotIndex, int32 SlotCount, float FollowDistance, float Spacing, int32 RowWidth);
}
