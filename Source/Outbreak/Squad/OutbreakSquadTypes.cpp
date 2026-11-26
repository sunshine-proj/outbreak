#include "Squad/OutbreakSquadTypes.h"

#define LOCTEXT_NAMESPACE "OutbreakSquad"

bool FOutbreakSquadSizeLimits::IsValid() const
{
	return MinSquadSize >= 1 && MinSquadSize <= DefaultSquadSize && DefaultSquadSize <= MaxSquadSize;
}

namespace OutbreakSquadRules
{
	bool ResolveLimits(const FOutbreakSquadSizeLimits& GlobalLimits, int32 MinOverride, int32 MaxOverride,
		FOutbreakSquadSizeLimits& OutLimits, FText& OutError)
	{
		if (!GlobalLimits.IsValid())
		{
			OutError = FText::Format(LOCTEXT("InvalidGlobal",
				"Project squad limits are invalid (min {0}, default {1}, max {2}); expected 1 <= min <= default <= max."),
				GlobalLimits.MinSquadSize, GlobalLimits.DefaultSquadSize, GlobalLimits.MaxSquadSize);
			return false;
		}

		FOutbreakSquadSizeLimits Limits = GlobalLimits;
		if (MinOverride > 0)
		{
			Limits.MinSquadSize = FMath::Clamp(MinOverride, GlobalLimits.MinSquadSize, GlobalLimits.MaxSquadSize);
		}
		if (MaxOverride > 0)
		{
			Limits.MaxSquadSize = FMath::Clamp(MaxOverride, GlobalLimits.MinSquadSize, GlobalLimits.MaxSquadSize);
		}

		if (Limits.MinSquadSize > Limits.MaxSquadSize)
		{
			OutError = FText::Format(LOCTEXT("EmptyRange",
				"Squad configuration limits leave no valid size (min {0} > max {1})."),
				Limits.MinSquadSize, Limits.MaxSquadSize);
			return false;
		}

		Limits.DefaultSquadSize = FMath::Clamp(GlobalLimits.DefaultSquadSize, Limits.MinSquadSize, Limits.MaxSquadSize);
		OutLimits = Limits;
		return true;
	}

	bool ResolveSquadSize(const FOutbreakSquadSizeLimits& Limits, int32 RequestedSize, int32& OutSquadSize, FText& OutError)
	{
		const int32 SquadSize = RequestedSize > 0 ? RequestedSize : Limits.DefaultSquadSize;
		if (!Limits.Contains(SquadSize))
		{
			OutError = FText::Format(LOCTEXT("SizeOutOfRange",
				"Squad size {0} is outside the allowed range {1}-{2} (the player is included)."),
				SquadSize, Limits.MinSquadSize, Limits.MaxSquadSize);
			return false;
		}

		OutSquadSize = SquadSize;
		return true;
	}
}

#undef LOCTEXT_NAMESPACE
