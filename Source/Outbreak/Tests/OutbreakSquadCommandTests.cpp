#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/OutbreakCombatTestTarget.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Game/OutbreakPlayerState.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Player/OutbreakSquadCommandComponent.h"
#include "Squad/OutbreakSquadCommandTypes.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadConfiguration.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakSquadSettings.h"
#include "Squad/OutbreakTeammateCharacter.h"
#include "Tests/OutbreakTestWorld.h"
#include "UObject/Package.h"

namespace OutbreakSquadCommandTests
{
	/** A squad of the player plus TeammateCount teammates, registered directly. */
	struct FTestSquad
	{
		AOutbreakPlayerState* PlayerState = nullptr;
		AOutbreakPlayerCharacter* Player = nullptr;
		TArray<AOutbreakTeammateCharacter*> Teammates;
		UOutbreakSquadComponent* Squad = nullptr;

		bool Create(FOutbreakTestWorld& TestWorld, int32 TeammateCount)
		{
			PlayerState = TestWorld.Spawn<AOutbreakPlayerState>();
			Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
			if (!PlayerState || !Player)
			{
				return false;
			}
			Squad = PlayerState->GetSquadComponent();
			Squad->RegisterPlayerMember(Player->GetSquadMemberComponent());
			for (int32 Index = 0; Index < TeammateCount; ++Index)
			{
				AOutbreakTeammateCharacter* Teammate = TestWorld.Spawn<AOutbreakTeammateCharacter>(FVector(-200.0f * (Index + 1), 0.0f, 0.0f));
				if (!Teammate)
				{
					return false;
				}
				Squad->RegisterMember(Teammate->GetSquadMemberComponent());
				Teammates.Add(Teammate);
			}
			return true;
		}

		UOutbreakSquadMemberComponent* Member(int32 Index) const { return Teammates[Index]->GetSquadMemberComponent(); }
	};

	int32 CountAccepted(const TArray<FOutbreakCommandResult>& Results)
	{
		int32 Count = 0;
		for (const FOutbreakCommandResult& Result : Results)
		{
			Count += Result.WasAccepted() ? 1 : 0;
		}
		return Count;
	}
}

using namespace OutbreakSquadCommandTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadCommandScopesTest, "Outbreak.Squad.Commands.Scopes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Commands reach the whole squad, one member, or a selection; never the player. */
bool FOutbreakSquadCommandScopesTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	FTestSquad Test;
	if (!TestTrue(TEXT("Squad created"), Test.Create(TestWorld, 3)))
	{
		return false;
	}

	const FOutbreakSquadCommand Follow = FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Follow);
	TArray<FOutbreakCommandResult> Results = Test.Squad->AuthIssueCommand(Follow, FOutbreakCommandRecipients::ForSquad());
	TestEqual(TEXT("Squad scope reaches every teammate"), Results.Num(), 3);
	TestEqual(TEXT("All accepted"), CountAccepted(Results), 3);
	TestTrue(TEXT("Player has no order"), Test.Player->GetSquadMemberComponent()->GetActiveCommandType() == EOutbreakSquadCommandType::None);

	Results = Test.Squad->AuthIssueCommand(FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Hold),
		FOutbreakCommandRecipients::ForMember(Test.Member(1)->GetMemberId()));
	TestEqual(TEXT("Individual scope reaches one"), Results.Num(), 1);
	TestTrue(TEXT("Individual got Hold"), Test.Member(1)->GetActiveCommandType() == EOutbreakSquadCommandType::Hold);
	TestTrue(TEXT("Others keep Follow"), Test.Member(0)->GetActiveCommandType() == EOutbreakSquadCommandType::Follow);
	TestTrue(TEXT("Hold without location holds in place"),
		Test.Member(1)->GetCurrentCommand().TargetLocation.Equals(Test.Member(1)->GetMemberLocation(), 1.0));

	Results = Test.Squad->AuthIssueCommand(FOutbreakSquadCommand::MakeMove(FVector(1000.0f, 0.0f, 0.0f)),
		FOutbreakCommandRecipients::ForMembers({ Test.Member(0)->GetMemberId(), Test.Member(2)->GetMemberId() }));
	TestEqual(TEXT("Selected scope reaches the selection"), CountAccepted(Results), 2);
	TestTrue(TEXT("Selected member 0 moves"), Test.Member(0)->GetActiveCommandType() == EOutbreakSquadCommandType::Move);
	TestTrue(TEXT("Unselected member 1 still holds"), Test.Member(1)->GetActiveCommandType() == EOutbreakSquadCommandType::Hold);
	TestTrue(TEXT("Selected member 2 moves"), Test.Member(2)->GetActiveCommandType() == EOutbreakSquadCommandType::Move);

	Results = Test.Squad->AuthIssueCommand(Follow,
		FOutbreakCommandRecipients::ForMember(Test.Player->GetSquadMemberComponent()->GetMemberId()));
	TestTrue(TEXT("Ordering the player is rejected"),
		Results.Num() == 1 && Results[0].Rejection == EOutbreakCommandRejection::PlayerMember);

	Results = Test.Squad->AuthIssueCommand(Follow, FOutbreakCommandRecipients::ForMember(9999));
	TestEqual(TEXT("Unknown IDs are skipped"), Results.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadCommandValidationTest, "Outbreak.Squad.Commands.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakSquadCommandValidationTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	FTestSquad Test;
	AOutbreakCombatTestTarget* Hostile = TestWorld.Spawn<AOutbreakCombatTestTarget>(FVector(1500.0f, 0.0f, 0.0f));
	if (!TestTrue(TEXT("Squad created"), Test.Create(TestWorld, 2)) || !Hostile)
	{
		return false;
	}

	UOutbreakSquadMemberComponent* Member = Test.Member(0);
	auto Validate = [&Test, Member](const FOutbreakSquadCommand& Command)
	{
		return Test.Squad->ValidateCommandForMember(Command, Member);
	};

	TestTrue(TEXT("None is not a command"), Validate(FOutbreakSquadCommand()) == EOutbreakCommandRejection::NoCommand);
	TestTrue(TEXT("Move needs a location"),
		Validate(FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Move)) == EOutbreakCommandRejection::MissingLocation);
	TestTrue(TEXT("Move with location"), Validate(FOutbreakSquadCommand::MakeMove(FVector(100, 0, 0))) == EOutbreakCommandRejection::None);
	TestTrue(TEXT("Attack needs a target"), Validate(FOutbreakSquadCommand::MakeAttack(nullptr)) == EOutbreakCommandRejection::InvalidTarget);
	TestTrue(TEXT("Attack a teammate refused"),
		Validate(FOutbreakSquadCommand::MakeAttack(Test.Teammates[1])) == EOutbreakCommandRejection::TargetNotHostile);
	TestTrue(TEXT("Attack the hostile target"), Validate(FOutbreakSquadCommand::MakeAttack(Hostile)) == EOutbreakCommandRejection::None);

	Hostile->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	TestTrue(TEXT("Attack a dead target refused"), Validate(FOutbreakSquadCommand::MakeAttack(Hostile)) == EOutbreakCommandRejection::InvalidTarget);

	TestTrue(TEXT("Non-member refused"),
		Test.Squad->ValidateCommandForMember(FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Follow), nullptr)
			== EOutbreakCommandRejection::NotInSquad);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadCommandLifecycleTest, "Outbreak.Squad.Commands.CancellationAndStatus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** New orders replace old ones; cancellation and member death end orders with the right status. */
bool FOutbreakSquadCommandLifecycleTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	FTestSquad Test;
	if (!TestTrue(TEXT("Squad created"), Test.Create(TestWorld, 2)))
	{
		return false;
	}

	UOutbreakSquadMemberComponent* Alpha = Test.Member(0);
	UOutbreakSquadMemberComponent* Bravo = Test.Member(1);

	Test.Squad->AuthIssueCommand(FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Follow), FOutbreakCommandRecipients::ForSquad());
	TestTrue(TEXT("Active after issue"), Alpha->GetCommandStatus() == EOutbreakSquadCommandStatus::Active);

	Test.Squad->AuthIssueCommand(FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Hold), FOutbreakCommandRecipients::ForSquad());
	TestTrue(TEXT("New order replaces the old"), Alpha->GetActiveCommandType() == EOutbreakSquadCommandType::Hold);

	TestEqual(TEXT("Cancel one"), Test.Squad->AuthCancelCommand(FOutbreakCommandRecipients::ForMember(Alpha->GetMemberId())), 1);
	TestTrue(TEXT("Cancelled status"), Alpha->GetCommandStatus() == EOutbreakSquadCommandStatus::Cancelled);
	TestTrue(TEXT("Cancelled member is autonomous"), Alpha->GetActiveCommandType() == EOutbreakSquadCommandType::None);
	TestTrue(TEXT("Other member unaffected"), Bravo->GetActiveCommandType() == EOutbreakSquadCommandType::Hold);
	TestEqual(TEXT("Nothing left to cancel on Alpha"), Test.Squad->AuthCancelCommand(FOutbreakCommandRecipients::ForMember(Alpha->GetMemberId())), 0);

	Test.Teammates[1]->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	TestTrue(TEXT("Death fails the order"), Bravo->GetCommandStatus() == EOutbreakSquadCommandStatus::Failed);

	const TArray<FOutbreakCommandResult> Results = Test.Squad->AuthIssueCommand(
		FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Follow), FOutbreakCommandRecipients::ForSquad());
	TestEqual(TEXT("Dead teammate still resolved"), Results.Num(), 2);
	TestEqual(TEXT("Only the living accept"), CountAccepted(Results), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadCommandSizesTest, "Outbreak.Squad.Commands.AllSquadSizes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Squad-wide and individual commands work for every supported squad size (Phase 4 acceptance). */
bool FOutbreakSquadCommandSizesTest::RunTest(const FString& Parameters)
{
	const FOutbreakSquadSizeLimits& Limits = GetDefault<UOutbreakSquadSettings>()->SizeLimits;
	for (int32 SquadSize = Limits.MinSquadSize; SquadSize <= Limits.MaxSquadSize; ++SquadSize)
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerState* PlayerState = TestWorld.Spawn<AOutbreakPlayerState>();
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		UOutbreakSquadConfiguration* Configuration = NewObject<UOutbreakSquadConfiguration>(GetTransientPackage());
		Configuration->SquadSize = SquadSize;

		UOutbreakSquadComponent* Squad = PlayerState->GetSquadComponent();
		if (!TestTrue(FString::Printf(TEXT("Squad of %d created"), SquadSize), Squad->InitializeSquad(Configuration, Player)))
		{
			continue;
		}

		const TArray<FOutbreakCommandResult> Results = Squad->AuthIssueCommand(
			FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Follow), FOutbreakCommandRecipients::ForSquad());
		TestEqual(FString::Printf(TEXT("Squad of %d: every teammate ordered"), SquadSize), CountAccepted(Results), SquadSize - 1);

		const TArray<UOutbreakSquadMemberComponent*> Teammates = Squad->GetTeammates();
		UOutbreakSquadMemberComponent* Last = Teammates.Last();
		Squad->AuthIssueCommand(FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Hold), FOutbreakCommandRecipients::ForMember(Last->GetMemberId()));
		TestTrue(FString::Printf(TEXT("Squad of %d: last teammate individually ordered"), SquadSize),
			Last->GetActiveCommandType() == EOutbreakSquadCommandType::Hold);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadSelectionTest, "Outbreak.Squad.Commands.Selection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakSquadSelectionTest::RunTest(const FString& Parameters)
{
	FOutbreakSquadSelection Selection;
	TestTrue(TEXT("Starts with all selected"), Selection.IsAllSelected());
	TestTrue(TEXT("All maps to squad scope"), Selection.ToRecipients().Scope == EOutbreakCommandScope::Squad);

	Selection.SelectOnly(3);
	TestTrue(TEXT("One selected: individual scope"), Selection.ToRecipients().Scope == EOutbreakCommandScope::Individual);
	TestTrue(TEXT("Selected member"), Selection.IsSelected(3));
	TestFalse(TEXT("Others not selected"), Selection.IsSelected(4));

	Selection.Toggle(4);
	TestTrue(TEXT("Two selected: selected scope"), Selection.ToRecipients().Scope == EOutbreakCommandScope::Selected);
	TestEqual(TEXT("Two IDs"), Selection.ToRecipients().MemberIds.Num(), 2);

	Selection.Toggle(3);
	Selection.Toggle(4);
	TestTrue(TEXT("Empty selection falls back to all"), Selection.IsAllSelected());

	Selection.SelectOnly(5);
	Selection.Prune({ 3, 4 });
	TestTrue(TEXT("Pruning removed members falls back to all"), Selection.IsAllSelected());

	Selection.Toggle(7);
	TestTrue(TEXT("Toggling from all starts a new set"), !Selection.IsAllSelected() && Selection.IsSelected(7));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadPanelTest, "Outbreak.Squad.Commands.StatusLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** The HUD line shows slot, name, order, activity, health and distance. */
bool FOutbreakSquadPanelTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	FTestSquad Test;
	if (!TestTrue(TEXT("Squad created"), Test.Create(TestWorld, 1)))
	{
		return false;
	}

	UOutbreakSquadMemberComponent* Member = Test.Member(0);
	Member->SetDisplayName(FText::FromString(TEXT("Hawk")));
	Test.Squad->AuthIssueCommand(FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Hold), FOutbreakCommandRecipients::ForSquad());

	const FString Line = UOutbreakSquadCommandComponent::FormatMemberLine(1, Member, true, 12.0f).ToString();
	TestTrue(TEXT("Shows selection marker"), Line.StartsWith(TEXT(">")));
	TestTrue(TEXT("Shows name"), Line.Contains(TEXT("Hawk")));
	TestTrue(TEXT("Shows order"), Line.Contains(TEXT("Hold")));
	TestTrue(TEXT("Shows health"), Line.Contains(TEXT("100%")));
	TestTrue(TEXT("Shows distance"), Line.Contains(TEXT("12 m")));

	Test.Teammates[0]->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	TestTrue(TEXT("Shows KIA"), UOutbreakSquadCommandComponent::FormatMemberLine(1, Member, false, 0.0f).ToString().Contains(TEXT("KIA")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
