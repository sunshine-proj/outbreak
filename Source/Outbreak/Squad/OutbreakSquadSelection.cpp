#include "Squad/OutbreakSquadSelection.h"

#include "Inventory/OutbreakLoadout.h"
#include "Squad/OutbreakSquadConfiguration.h"
#include "Squad/OutbreakSquadSettings.h"

#define LOCTEXT_NAMESPACE "OutbreakSquadSelection"

namespace OutbreakSquadSelection
{
	namespace
	{
		int32 RosterSize(const UOutbreakSquadConfiguration* Configuration)
		{
			return Configuration ? Configuration->Teammates.Num() : 0;
		}

		bool IsUsedByOtherSlot(const FOutbreakSquadSelection& Selection, int32 RosterIndex, int32 ExceptSlot)
		{
			for (int32 Slot = 0; Slot < Selection.TeammateRosterIndices.Num(); ++Slot)
			{
				if (Slot != ExceptSlot && Selection.TeammateRosterIndices[Slot] == RosterIndex)
				{
					return true;
				}
			}
			return false;
		}

		int32 FirstUnusedRosterIndex(const UOutbreakSquadConfiguration* Configuration, const FOutbreakSquadSelection& Selection)
		{
			for (int32 RosterIndex = 0; RosterIndex < RosterSize(Configuration); ++RosterIndex)
			{
				if (!Selection.TeammateRosterIndices.Contains(RosterIndex))
				{
					return RosterIndex;
				}
			}
			return INDEX_NONE;
		}

		/** Steps through Options from Current by Direction, wrapping. Current not found = the first option. */
		template <typename T>
		T Step(const TArray<T>& Options, const T& Current, int32 Direction)
		{
			const int32 CurrentIndex = Options.IndexOfByKey(Current);
			if (CurrentIndex == INDEX_NONE)
			{
				return Options[0];
			}
			const int32 Count = Options.Num();
			return Options[((CurrentIndex + (Direction >= 0 ? 1 : -1)) % Count + Count) % Count];
		}
	}

	FOutbreakSquadSizeLimits ResolveLimits(const UOutbreakSquadConfiguration* Configuration)
	{
		const FOutbreakSquadSizeLimits& Global = GetDefault<UOutbreakSquadSettings>()->SizeLimits;
		FOutbreakSquadSizeLimits Resolved;
		FText Error;
		if (OutbreakSquadRules::ResolveLimits(Global,
			Configuration ? Configuration->MinSquadSizeOverride : 0,
			Configuration ? Configuration->MaxSquadSizeOverride : 0,
			Resolved, Error))
		{
			return Resolved;
		}
		return Global.IsValid() ? Global : FOutbreakSquadSizeLimits();
	}

	FOutbreakSquadSelection MakeDefault(const UOutbreakSquadConfiguration* Configuration, const FOutbreakSquadSizeLimits& Limits,
		int32 RequestedSize)
	{
		int32 Size = RequestedSize > 0 ? RequestedSize
			: (Configuration && Configuration->SquadSize > 0 ? Configuration->SquadSize : Limits.DefaultSquadSize);
		Size = FMath::Clamp(Size, Limits.MinSquadSize, Limits.MaxSquadSize);

		FOutbreakSquadSelection Selection;
		Selection.SquadSize = Size;
		for (int32 Slot = 0; Slot < Size - 1; ++Slot)
		{
			Selection.TeammateRosterIndices.Add(Slot < RosterSize(Configuration) ? Slot : INDEX_NONE);
		}
		Selection.MemberLoadouts.SetNum(Size);
		return Selection;
	}

	bool Validate(const UOutbreakSquadConfiguration* Configuration, const FOutbreakSquadSizeLimits& Limits,
		const FOutbreakSquadSelection& Selection, TArray<FText>& OutErrors)
	{
		const int32 InitialErrorCount = OutErrors.Num();

		int32 ResolvedSize = 0;
		FText SizeError;
		if (Selection.SquadSize <= 0 || !OutbreakSquadRules::ResolveSquadSize(Limits, Selection.SquadSize, ResolvedSize, SizeError))
		{
			OutErrors.Add(FText::Format(LOCTEXT("BadSize", "Squad size {0} is outside {1}-{2}."),
				Selection.SquadSize, Limits.MinSquadSize, Limits.MaxSquadSize));
		}
		if (Selection.TeammateRosterIndices.Num() != FMath::Max(0, Selection.SquadSize - 1))
		{
			OutErrors.Add(FText::Format(LOCTEXT("SlotCount", "A squad of {0} needs {1} teammate slots, not {2}."),
				Selection.SquadSize, FMath::Max(0, Selection.SquadSize - 1), Selection.TeammateRosterIndices.Num()));
		}

		TSet<int32> Seen;
		for (const int32 RosterIndex : Selection.TeammateRosterIndices)
		{
			if (RosterIndex == INDEX_NONE)
			{
				continue;
			}
			if (RosterIndex < 0 || RosterIndex >= RosterSize(Configuration))
			{
				OutErrors.Add(FText::Format(LOCTEXT("BadRosterIndex", "Roster soldier {0} does not exist."), RosterIndex));
			}
			else if (Seen.Contains(RosterIndex))
			{
				OutErrors.Add(FText::Format(LOCTEXT("DuplicateRoster", "Roster soldier {0} is chosen twice."), RosterIndex));
			}
			Seen.Add(RosterIndex);
		}

		if (Selection.MemberLoadouts.Num() > Selection.SquadSize)
		{
			OutErrors.Add(LOCTEXT("TooManyLoadouts", "More loadouts than squad members."));
		}
		for (const TObjectPtr<UOutbreakLoadoutDefinition>& Loadout : Selection.MemberLoadouts)
		{
			if (Loadout && (!Configuration || !Configuration->AvailableLoadouts.Contains(Loadout)))
			{
				OutErrors.Add(FText::Format(LOCTEXT("UnavailableLoadout", "Loadout {0} is not available for this squad."),
					FText::FromString(GetNameSafe(Loadout))));
			}
		}

		return OutErrors.Num() == InitialErrorCount;
	}

	void SetSquadSize(const UOutbreakSquadConfiguration* Configuration, const FOutbreakSquadSizeLimits& Limits,
		FOutbreakSquadSelection& Selection, int32 NewSize)
	{
		NewSize = FMath::Clamp(NewSize, Limits.MinSquadSize, Limits.MaxSquadSize);
		Selection.SquadSize = NewSize;

		const int32 SlotCount = FMath::Max(0, NewSize - 1);
		if (Selection.TeammateRosterIndices.Num() > SlotCount)
		{
			Selection.TeammateRosterIndices.SetNum(SlotCount);
		}
		while (Selection.TeammateRosterIndices.Num() < SlotCount)
		{
			Selection.TeammateRosterIndices.Add(FirstUnusedRosterIndex(Configuration, Selection));
		}
		Selection.MemberLoadouts.SetNum(NewSize);
	}

	void CycleTeammate(const UOutbreakSquadConfiguration* Configuration, FOutbreakSquadSelection& Selection, int32 Slot, int32 Direction)
	{
		if (!Selection.TeammateRosterIndices.IsValidIndex(Slot))
		{
			return;
		}
		TArray<int32> Options;
		for (int32 RosterIndex = 0; RosterIndex < RosterSize(Configuration); ++RosterIndex)
		{
			if (!IsUsedByOtherSlot(Selection, RosterIndex, Slot))
			{
				Options.Add(RosterIndex);
			}
		}
		Options.Add(INDEX_NONE);
		Selection.TeammateRosterIndices[Slot] = Step(Options, Selection.TeammateRosterIndices[Slot], Direction);
	}

	void CycleLoadout(const UOutbreakSquadConfiguration* Configuration, FOutbreakSquadSelection& Selection, int32 MemberIndex, int32 Direction)
	{
		if (MemberIndex < 0 || MemberIndex >= Selection.SquadSize)
		{
			return;
		}
		TArray<UOutbreakLoadoutDefinition*> Options;
		Options.Add(nullptr);
		if (Configuration)
		{
			for (UOutbreakLoadoutDefinition* Loadout : Configuration->AvailableLoadouts)
			{
				if (Loadout)
				{
					Options.AddUnique(Loadout);
				}
			}
		}
		if (Selection.MemberLoadouts.Num() < Selection.SquadSize)
		{
			Selection.MemberLoadouts.SetNum(Selection.SquadSize);
		}
		Selection.MemberLoadouts[MemberIndex] = Step(Options, Selection.MemberLoadouts[MemberIndex].Get(), Direction);
	}

	const FOutbreakSquadMemberConfig& GetTeammateConfig(const UOutbreakSquadConfiguration* Configuration,
		const FOutbreakSquadSelection& Selection, int32 Slot)
	{
		static const FOutbreakSquadMemberConfig EmptyConfig;
		if (!Configuration)
		{
			return EmptyConfig;
		}
		const int32 RosterIndex = Selection.TeammateRosterIndices.IsValidIndex(Slot) ? Selection.TeammateRosterIndices[Slot] : INDEX_NONE;
		return Configuration->Teammates.IsValidIndex(RosterIndex) ? Configuration->Teammates[RosterIndex] : Configuration->FillerTeammate;
	}

	UOutbreakLoadoutDefinition* GetLoadout(const FOutbreakSquadSelection& Selection, int32 MemberIndex)
	{
		return Selection.MemberLoadouts.IsValidIndex(MemberIndex) ? Selection.MemberLoadouts[MemberIndex].Get() : nullptr;
	}
}

#undef LOCTEXT_NAMESPACE
