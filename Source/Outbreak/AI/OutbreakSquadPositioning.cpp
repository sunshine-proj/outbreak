#include "AI/OutbreakSquadPositioning.h"

namespace OutbreakSquadPositioning
{
	FVector ComputeFollowOffset(int32 SlotIndex, int32 SlotCount, float FollowDistance, float Spacing, int32 RowWidth)
	{
		RowWidth = FMath::Max(1, RowWidth);
		SlotCount = FMath::Max(1, SlotCount);
		SlotIndex = FMath::Clamp(SlotIndex, 0, SlotCount - 1);

		const int32 Row = SlotIndex / RowWidth;
		const int32 Column = SlotIndex % RowWidth;
		const int32 SlotsInRow = FMath::Min(RowWidth, SlotCount - Row * RowWidth);

		const float Lateral = (Column - (SlotsInRow - 1) * 0.5f) * Spacing;
		const float Back = FollowDistance + Row * Spacing;
		return FVector(-Back, Lateral, 0.0f);
	}

	FVector ComputeFollowLocation(const FVector& LeaderLocation, const FRotator& LeaderRotation,
		int32 SlotIndex, int32 SlotCount, float FollowDistance, float Spacing, int32 RowWidth)
	{
		const FRotator YawOnly(0.0f, LeaderRotation.Yaw, 0.0f);
		return LeaderLocation + YawOnly.RotateVector(ComputeFollowOffset(SlotIndex, SlotCount, FollowDistance, Spacing, RowWidth));
	}
}
