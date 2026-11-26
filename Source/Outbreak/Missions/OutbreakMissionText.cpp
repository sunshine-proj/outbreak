#include "Missions/OutbreakMissionText.h"

#include "Inventory/OutbreakLoadout.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "Missions/OutbreakObjective.h"
#include "Missions/OutbreakObjectiveQueries.h"
#include "Squad/OutbreakSquadConfiguration.h"
#include "Squad/OutbreakSquadSelection.h"

#define LOCTEXT_NAMESPACE "OutbreakMissionText"

namespace OutbreakMissionText
{
	namespace
	{
		FText Join(const TArray<FText>& Lines)
		{
			return FText::Join(FText::FromString(TEXT("\n")), Lines);
		}

		FText StateMark(EOutbreakObjectiveState State)
		{
			switch (State)
			{
			case EOutbreakObjectiveState::Completed: return LOCTEXT("MarkCompleted", "[x]");
			case EOutbreakObjectiveState::Failed:    return LOCTEXT("MarkFailed", "[!]");
			case EOutbreakObjectiveState::Active:    return LOCTEXT("MarkActive", "[ ]");
			default:                                 return LOCTEXT("MarkPending", "[-]");
			}
		}

		FText ObjectiveName(const FOutbreakObjectiveStatus& Objective)
		{
			return Objective.bOptional
				? FText::Format(LOCTEXT("OptionalName", "(Optional) {0}"), Objective.Description)
				: Objective.Description;
		}

		FText ObjectiveLine(const FOutbreakObjectiveStatus& Objective)
		{
			return Objective.Progress.IsEmpty()
				? FText::Format(LOCTEXT("ObjectiveLine", "{0} {1}"), StateMark(Objective.State), ObjectiveName(Objective))
				: FText::Format(LOCTEXT("ObjectiveLineProgress", "{0} {1}  ({2})"), StateMark(Objective.State), ObjectiveName(Objective), Objective.Progress);
		}

		bool IsRevealed(const FOutbreakObjectiveStatus& Objective)
		{
			return !Objective.bHidden || Objective.State != EOutbreakObjectiveState::Inactive;
		}

		FText MemberOutcome(const FOutbreakMissionMemberReport& Member)
		{
			switch (Member.Status)
			{
			case EOutbreakSquadMemberStatus::Dead:          return LOCTEXT("OutcomeKIA", "KIA");
			case EOutbreakSquadMemberStatus::Incapacitated: return LOCTEXT("OutcomeDown", "Down, left behind");
			default:
				return Member.bExtracted ? LOCTEXT("OutcomeExtracted", "Extracted") : LOCTEXT("OutcomeActive", "Active");
			}
		}
	}

	FText BuildObjectiveList(const FOutbreakMissionStatus& Status, bool bExpanded)
	{
		TArray<FText> Lines;
		if (bExpanded)
		{
			Lines.Add(FText::Format(LOCTEXT("ObjectivesHeader", "OBJECTIVES: {0}"), Status.MissionName));
			for (const FOutbreakObjectiveStatus& Objective : Status.Objectives)
			{
				if (IsRevealed(Objective))
				{
					Lines.Add(ObjectiveLine(Objective));
				}
			}
			return Join(Lines);
		}

		// Compact: required objectives first, then optional ones.
		for (const bool bOptionalPass : { false, true })
		{
			for (const FOutbreakObjectiveStatus& Objective : Status.Objectives)
			{
				if (Objective.State == EOutbreakObjectiveState::Active && Objective.bOptional == bOptionalPass)
				{
					Lines.Add(ObjectiveLine(Objective));
				}
			}
		}
		return Join(Lines);
	}

	TArray<FText> BuildObjectiveNotifications(const FOutbreakMissionStatus& Previous, const FOutbreakMissionStatus& Current)
	{
		TArray<FText> Notifications;
		// A different objective list (another mission, or the first status) has no comparable history.
		const bool bComparable = Previous.Objectives.Num() == Current.Objectives.Num();
		if (Current.State != EOutbreakMissionState::InProgress
			&& !(Previous.State == EOutbreakMissionState::InProgress && bComparable))
		{
			return Notifications;
		}

		for (int32 Index = 0; Index < Current.Objectives.Num(); ++Index)
		{
			const FOutbreakObjectiveStatus& Objective = Current.Objectives[Index];
			const EOutbreakObjectiveState OldState = bComparable ? Previous.Objectives[Index].State : EOutbreakObjectiveState::Inactive;
			// A hidden objective cancelled before it was ever revealed stays secret.
			if (Objective.State == OldState || (Objective.bHidden && OldState == EOutbreakObjectiveState::Inactive
				&& Objective.State == EOutbreakObjectiveState::Failed))
			{
				continue;
			}
			switch (Objective.State)
			{
			case EOutbreakObjectiveState::Active:
				Notifications.Add(Objective.bOptional
					? FText::Format(LOCTEXT("NewOptional", "Optional objective: {0}"), Objective.Description)
					: FText::Format(LOCTEXT("NewObjective", "New objective: {0}"), Objective.Description));
				break;
			case EOutbreakObjectiveState::Completed:
				Notifications.Add(FText::Format(LOCTEXT("ObjectiveDone", "Objective complete: {0}"), Objective.Description));
				break;
			case EOutbreakObjectiveState::Failed:
				// An optional objective that never started was cancelled by a failed prerequisite; still worth saying.
				Notifications.Add(Objective.bOptional
					? FText::Format(LOCTEXT("OptionalFailed", "Optional objective failed: {0}"), Objective.Description)
					: FText::Format(LOCTEXT("ObjectiveFailedNote", "Objective failed: {0}"), Objective.Description));
				break;
			default:
				break;
			}
		}
		return Notifications;
	}

	FText BuildBriefing(const UOutbreakMissionDefinition* Mission)
	{
		if (!Mission)
		{
			return FText::GetEmpty();
		}

		TArray<FText> Lines;
		if (!Mission->Description.IsEmpty())
		{
			Lines.Add(Mission->Description);
			Lines.Add(FText::GetEmpty());
		}
		if (!Mission->Briefing.IsEmpty())
		{
			Lines.Add(Mission->Briefing);
			Lines.Add(FText::GetEmpty());
		}

		Lines.Add(LOCTEXT("BriefingObjectives", "OBJECTIVES"));
		for (const UOutbreakObjective* Objective : Mission->Objectives)
		{
			if (Objective && !Objective->IsHidden())
			{
				Lines.Add(Objective->IsOptional()
					? FText::Format(LOCTEXT("BriefingOptional", "- (Optional) {0}"), Objective->GetDescription())
					: FText::Format(LOCTEXT("BriefingRequired", "- {0}"), Objective->GetDescription()));
			}
		}
		return Join(Lines);
	}

	FText GetTeammateName(const UOutbreakSquadConfiguration* Configuration, const FOutbreakDeploymentSelection& Selection, int32 Slot)
	{
		const FText& Name = OutbreakSquadSelection::GetTeammateConfig(Configuration, Selection, Slot).DisplayName;
		return Name.IsEmpty() ? LOCTEXT("Reinforcement", "Reinforcement") : Name;
	}

	FText GetLoadoutName(const FOutbreakDeploymentSelection& Selection, int32 MemberIndex)
	{
		const UOutbreakLoadoutDefinition* Loadout = OutbreakSquadSelection::GetLoadout(Selection, MemberIndex);
		if (!Loadout)
		{
			return LOCTEXT("DefaultEquipment", "Default equipment");
		}
		return Loadout->DisplayName.IsEmpty() ? FText::FromString(Loadout->GetName()) : Loadout->DisplayName;
	}

	FText BuildResultTitle(const FOutbreakMissionResults& Results)
	{
		return Results.Outcome == EOutbreakMissionState::Completed
			? LOCTEXT("TitleComplete", "MISSION COMPLETE")
			: LOCTEXT("TitleFailed", "MISSION FAILED");
	}

	FText BuildSummary(const FOutbreakMissionResults& Results)
	{
		TArray<FText> Lines;
		Lines.Add(Results.MissionName);
		Lines.Add(Results.ResultText);
		Lines.Add(FText::Format(LOCTEXT("SummaryTime", "Mission time: {0}"), OutbreakObjectiveQueries::FormatTime(Results.Duration)));
		Lines.Add(FText::GetEmpty());

		Lines.Add(FText::Format(LOCTEXT("SummaryObjectives", "OBJECTIVES  {0} / {1} required"),
			Results.RequiredObjectivesCompleted, Results.RequiredObjectiveCount));
		if (Results.OptionalObjectiveCount > 0)
		{
			Lines.Last() = FText::Format(LOCTEXT("SummaryObjectivesOptional", "{0}, {1} / {2} optional"),
				Lines.Last(), Results.OptionalObjectivesCompleted, Results.OptionalObjectiveCount);
		}
		for (const FOutbreakObjectiveStatus& Objective : Results.Objectives)
		{
			if (IsRevealed(Objective))
			{
				FOutbreakObjectiveStatus Final = Objective;
				Final.Progress = FText::GetEmpty();
				Lines.Add(ObjectiveLine(Final));
			}
		}

		if (!Results.Squad.IsEmpty())
		{
			Lines.Add(FText::GetEmpty());
			Lines.Add(LOCTEXT("SummarySquad", "SQUAD"));
			for (const FOutbreakMissionMemberReport& Member : Results.Squad)
			{
				Lines.Add(FText::Format(LOCTEXT("SummaryMember", "{0}{1}: {2}"), Member.DisplayName,
					Member.bIsPlayer ? LOCTEXT("PlayerSuffix", " (you)") : FText::GetEmpty(), MemberOutcome(Member)));
			}
		}
		return Join(Lines);
	}
}

#undef LOCTEXT_NAMESPACE
