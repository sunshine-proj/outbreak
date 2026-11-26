#include "AI/OutbreakSquadPositioning.h"

#define LOCTEXT_NAMESPACE "OutbreakFormations"

namespace OutbreakSquadPositioning
{
	namespace
	{
		/** Deterministic per-slot jitter in [-1, 1], so Loose looks irregular but is stable. */
		float SlotJitter(int32 SlotIndex, int32 Salt)
		{
			const uint32 Hash = HashCombine(GetTypeHash(SlotIndex), GetTypeHash(Salt));
			return (static_cast<float>(Hash % 2001) / 1000.0f) - 1.0f;
		}

		/** Slot i alternates right/left and moves one step outward every two slots. */
		void AlternatingSide(int32 SlotIndex, float& OutSide, int32& OutStep)
		{
			OutSide = (SlotIndex % 2 == 0) ? 1.0f : -1.0f;
			OutStep = SlotIndex / 2 + 1;
		}
	}

	FVector ComputeFormationOffset(EOutbreakFormation Formation, int32 SlotIndex, int32 SlotCount, float Spacing)
	{
		SlotCount = FMath::Max(1, SlotCount);
		SlotIndex = FMath::Clamp(SlotIndex, 0, SlotCount - 1);

		float Side = 1.0f;
		int32 Step = 1;
		AlternatingSide(SlotIndex, Side, Step);

		switch (Formation)
		{
		case EOutbreakFormation::Column:
			return FVector(-Spacing * (SlotIndex + 1), 0.0f, 0.0f);

		case EOutbreakFormation::Line:
			// Slightly behind the leader so teammates don't walk into its line of fire.
			return FVector(-Spacing * 0.25f, Side * Spacing * Step, 0.0f);

		case EOutbreakFormation::Defensive:
		{
			// Evenly spaced on a ring; the ring grows so neighbours stay one spacing apart.
			const float Radius = FMath::Max(Spacing, SlotCount * Spacing / (2.0f * PI));
			const float Angle = PI + 2.0f * PI * SlotIndex / SlotCount;   // Slot 0 directly behind.
			return FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
		}

		case EOutbreakFormation::Loose:
		{
			const float LooseSpacing = Spacing * 1.75f;
			return FVector(-LooseSpacing * Step + SlotJitter(SlotIndex, 1) * Spacing * 0.25f,
				Side * LooseSpacing * Step + SlotJitter(SlotIndex, 2) * Spacing * 0.25f, 0.0f);
		}

		case EOutbreakFormation::Stealth:
			// Tight file, alternating slightly left and right so teammates don't block each other's view.
			return FVector(-Spacing * (SlotIndex + 1), (SlotIndex % 2 == 0 ? 0.3f : -0.3f) * Spacing, 0.0f);

		case EOutbreakFormation::Wedge:
		default:
			return FVector(-Spacing * Step, Side * Spacing * Step, 0.0f);
		}
	}

	FVector ComputeFormationLocation(const FVector& LeaderLocation, const FRotator& LeaderRotation,
		EOutbreakFormation Formation, int32 SlotIndex, int32 SlotCount, float Spacing)
	{
		const FRotator YawOnly(0.0f, LeaderRotation.Yaw, 0.0f);
		return LeaderLocation + YawOnly.RotateVector(ComputeFormationOffset(Formation, SlotIndex, SlotCount, Spacing));
	}

	FText GetFormationDisplayName(EOutbreakFormation Formation)
	{
		switch (Formation)
		{
		case EOutbreakFormation::Column:    return LOCTEXT("Column", "Column");
		case EOutbreakFormation::Line:      return LOCTEXT("Line", "Line");
		case EOutbreakFormation::Defensive: return LOCTEXT("Defensive", "Defensive");
		case EOutbreakFormation::Loose:     return LOCTEXT("Loose", "Loose");
		case EOutbreakFormation::Stealth:   return LOCTEXT("Stealth", "Stealth");
		default:                            return LOCTEXT("Wedge", "Wedge");
		}
	}
}

#undef LOCTEXT_NAMESPACE
