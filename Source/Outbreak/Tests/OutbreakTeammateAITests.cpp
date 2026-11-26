#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/OutbreakSquadAIController.h"
#include "AI/OutbreakSquadPositioning.h"
#include "AI/OutbreakTeammateBehaviorTypes.h"
#include "AI/OutbreakTeammateCombatRules.h"
#include "Combat/OutbreakCombatTestTarget.h"
#include "Combat/OutbreakTeam.h"
#include "Game/OutbreakPlayerState.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakTeammateCharacter.h"
#include "Tests/OutbreakTestWorld.h"

namespace OutbreakTeammateAITests
{
	FOutbreakBehaviorContext MakeContext()
	{
		FOutbreakBehaviorContext Context;
		Context.bHasPlayer = true;
		Context.FollowStartDistance = 250.0f;
		Context.FollowStopDistance = 100.0f;
		return Context;
	}

	EOutbreakTeammateBehavior Select(const FOutbreakBehaviorContext& Context)
	{
		return OutbreakTeammateBehavior::SelectBehavior(Context, OutbreakTeammateBehavior::GetDefaultPriorityOrder()).Behavior;
	}
}

using namespace OutbreakTeammateAITests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakBehaviorPriorityTest, "Outbreak.AI.Behavior.DefaultPriorities",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Emergency survival > explicit order > immediate threat > formation > default (AGENTS.md §6.1). */
bool FOutbreakBehaviorPriorityTest::RunTest(const FString& Parameters)
{
	FOutbreakBehaviorContext Context = MakeContext();
	TestTrue(TEXT("Nothing to do: idle"), Select(Context) == EOutbreakTeammateBehavior::Idle);

	Context.DistanceToFollowLocation = 1000.0f;
	TestTrue(TEXT("Far from slot: follow"), Select(Context) == EOutbreakTeammateBehavior::FollowPlayer);

	Context.bHasThreat = true;
	TestTrue(TEXT("Threat beats formation"), Select(Context) == EOutbreakTeammateBehavior::EngageThreat);

	Context.ActiveCommand = EOutbreakSquadCommandType::Hold;
	TestTrue(TEXT("Explicit order beats threat"), Select(Context) == EOutbreakTeammateBehavior::HoldPosition);

	Context.bLowHealth = true;
	TestTrue(TEXT("Survival beats explicit order"), Select(Context) == EOutbreakTeammateBehavior::SeekSafety);

	Context.bHasThreat = false;
	TestTrue(TEXT("Low health without threat keeps the order"), Select(Context) == EOutbreakTeammateBehavior::HoldPosition);

	const TPair<EOutbreakSquadCommandType, EOutbreakTeammateBehavior> OrderMap[] = {
		{ EOutbreakSquadCommandType::Follow, EOutbreakTeammateBehavior::FollowPlayer },
		{ EOutbreakSquadCommandType::Hold, EOutbreakTeammateBehavior::HoldPosition },
		{ EOutbreakSquadCommandType::Move, EOutbreakTeammateBehavior::MoveToLocation },
		{ EOutbreakSquadCommandType::Regroup, EOutbreakTeammateBehavior::Regroup },
		{ EOutbreakSquadCommandType::Attack, EOutbreakTeammateBehavior::AttackTarget },
	};
	FOutbreakBehaviorContext OrderContext = MakeContext();
	OrderContext.bHasThreat = true;
	for (const TPair<EOutbreakSquadCommandType, EOutbreakTeammateBehavior>& Entry : OrderMap)
	{
		OrderContext.ActiveCommand = Entry.Key;
		TestTrue(FString::Printf(TEXT("Order %d maps to its behaviour"), static_cast<int32>(Entry.Key)), Select(OrderContext) == Entry.Value);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakBehaviorConfigurableOrderTest, "Outbreak.AI.Behavior.ConfigurablePriorities",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakBehaviorConfigurableOrderTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Default order is valid"), OutbreakTeammateBehavior::IsValidPriorityOrder(OutbreakTeammateBehavior::GetDefaultPriorityOrder()));

	// A designer can let threats override orders.
	const TArray<EOutbreakBehaviorLayer> ThreatFirst = {
		EOutbreakBehaviorLayer::EmergencySurvival,
		EOutbreakBehaviorLayer::ThreatResponse,
		EOutbreakBehaviorLayer::ExplicitOrder,
		EOutbreakBehaviorLayer::FormationMaintenance,
		EOutbreakBehaviorLayer::DefaultAutonomous
	};
	TestTrue(TEXT("Reordered list is valid"), OutbreakTeammateBehavior::IsValidPriorityOrder(ThreatFirst));

	FOutbreakBehaviorContext Context = MakeContext();
	Context.bHasThreat = true;
	Context.ActiveCommand = EOutbreakSquadCommandType::Hold;
	const FOutbreakBehaviorDecision Decision = OutbreakTeammateBehavior::SelectBehavior(Context, ThreatFirst);
	TestTrue(TEXT("Threat now beats the order"), Decision.Behavior == EOutbreakTeammateBehavior::EngageThreat);
	TestTrue(TEXT("Decision reports its layer"), Decision.Layer == EOutbreakBehaviorLayer::ThreatResponse);

	TestFalse(TEXT("Missing layer invalid"), OutbreakTeammateBehavior::IsValidPriorityOrder({
		EOutbreakBehaviorLayer::ExplicitOrder, EOutbreakBehaviorLayer::DefaultAutonomous }));
	TestFalse(TEXT("Duplicate layer invalid"), OutbreakTeammateBehavior::IsValidPriorityOrder({
		EOutbreakBehaviorLayer::ExplicitOrder, EOutbreakBehaviorLayer::ExplicitOrder, EOutbreakBehaviorLayer::ThreatResponse,
		EOutbreakBehaviorLayer::FormationMaintenance, EOutbreakBehaviorLayer::DefaultAutonomous }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakFollowHysteresisTest, "Outbreak.AI.Behavior.PlayerDistanceManagement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Start following beyond the start distance; keep following until inside the stop distance. */
bool FOutbreakFollowHysteresisTest::RunTest(const FString& Parameters)
{
	FOutbreakBehaviorContext Context = MakeContext();

	Context.DistanceToFollowLocation = 200.0f;
	Context.bCurrentlyFollowing = false;
	TestTrue(TEXT("Inside start distance: stay"), Select(Context) == EOutbreakTeammateBehavior::Idle);

	Context.DistanceToFollowLocation = 300.0f;
	TestTrue(TEXT("Beyond start distance: follow"), Select(Context) == EOutbreakTeammateBehavior::FollowPlayer);

	Context.DistanceToFollowLocation = 200.0f;
	Context.bCurrentlyFollowing = true;
	TestTrue(TEXT("Already following: continue"), Select(Context) == EOutbreakTeammateBehavior::FollowPlayer);

	Context.DistanceToFollowLocation = 50.0f;
	TestTrue(TEXT("Inside stop distance: stop"), Select(Context) == EOutbreakTeammateBehavior::Idle);

	Context.bHasPlayer = false;
	Context.DistanceToFollowLocation = 1000.0f;
	TestTrue(TEXT("No player: nothing to follow"), Select(Context) == EOutbreakTeammateBehavior::Idle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakFollowPositionsTest, "Outbreak.AI.Positioning.FollowSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Follow slots are behind the player, distinct, and work for any number of teammates. */
bool FOutbreakFollowPositionsTest::RunTest(const FString& Parameters)
{
	for (int32 Count = 1; Count <= 9; ++Count)
	{
		TArray<FVector> Offsets;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Offset = OutbreakSquadPositioning::ComputeFollowOffset(Index, Count, 300.0f, 150.0f, 3);
			TestTrue(FString::Printf(TEXT("%d teammates: slot %d behind the player"), Count, Index), Offset.X <= -300.0f);
			for (const FVector& Other : Offsets)
			{
				TestTrue(FString::Printf(TEXT("%d teammates: slot %d distinct"), Count, Index), FVector::Dist(Offset, Other) >= 149.0f);
			}
			Offsets.Add(Offset);
		}
	}

	const FVector Single = OutbreakSquadPositioning::ComputeFollowOffset(0, 1, 300.0f, 150.0f, 3);
	TestEqual(TEXT("A single teammate is centred"), static_cast<double>(Single.Y), 0.0, 0.01);

	// The slot rotates with the player's yaw: facing +Y, "behind" is -Y.
	const FVector World = OutbreakSquadPositioning::ComputeFollowLocation(FVector::ZeroVector, FRotator(0.0f, 90.0f, 0.0f), 0, 1, 300.0f, 150.0f, 3);
	TestEqual(TEXT("Rotated slot X"), static_cast<double>(World.X), 0.0, 0.1);
	TestEqual(TEXT("Rotated slot Y"), static_cast<double>(World.Y), -300.0, 0.1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakTeammateCombatRulesTest, "Outbreak.AI.Combat.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakTeammateCombatRulesTest::RunTest(const FString& Parameters)
{
	const FVector Origin = FVector::ZeroVector;
	TestTrue(TEXT("Aimed straight at"), OutbreakTeammateCombat::IsAimedAt(Origin, FVector::ForwardVector, FVector(1000, 0, 0), 5.0f));
	TestTrue(TEXT("Within tolerance"), OutbreakTeammateCombat::IsAimedAt(Origin, FVector::ForwardVector, FVector(1000, 50, 0), 5.0f));
	TestFalse(TEXT("Outside tolerance"), OutbreakTeammateCombat::IsAimedAt(Origin, FVector::ForwardVector, FVector(1000, 500, 0), 5.0f));
	TestFalse(TEXT("Behind"), OutbreakTeammateCombat::IsAimedAt(Origin, FVector::ForwardVector, FVector(-1000, 0, 0), 5.0f));

	TestTrue(TEXT("Fire when everything allows"), OutbreakTeammateCombat::ShouldFire(true, 1000.0f, 3000.0f, true, true));
	TestFalse(TEXT("Not visible"), OutbreakTeammateCombat::ShouldFire(false, 1000.0f, 3000.0f, true, true));
	TestFalse(TEXT("Out of range"), OutbreakTeammateCombat::ShouldFire(true, 4000.0f, 3000.0f, true, true));
	TestFalse(TEXT("Not aimed"), OutbreakTeammateCombat::ShouldFire(true, 1000.0f, 3000.0f, false, true));
	TestFalse(TEXT("Weapon cannot fire"), OutbreakTeammateCombat::ShouldFire(true, 1000.0f, 3000.0f, true, false));

	TestTrue(TEXT("Empty: reload even in combat"), OutbreakTeammateCombat::ShouldReload(0, 30, true, 0.5f, true));
	TestFalse(TEXT("Partial in combat: keep firing"), OutbreakTeammateCombat::ShouldReload(5, 30, true, 0.5f, true));
	TestTrue(TEXT("Low and no target: top up"), OutbreakTeammateCombat::ShouldReload(10, 30, false, 0.5f, true));
	TestFalse(TEXT("Plenty and no target: no reload"), OutbreakTeammateCombat::ShouldReload(20, 30, false, 0.5f, true));
	TestFalse(TEXT("Cannot reload (no reserve)"), OutbreakTeammateCombat::ShouldReload(0, 30, false, 0.5f, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakTeamAttitudeTest, "Outbreak.AI.TeamAttitude",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakTeamAttitudeTest::RunTest(const FString& Parameters)
{
	const FGenericTeamId Player = OutbreakTeam::ToGenericTeamId(EOutbreakTeam::Player);
	const FGenericTeamId Hostile = OutbreakTeam::ToGenericTeamId(EOutbreakTeam::Hostile);
	const FGenericTeamId Neutral = OutbreakTeam::ToGenericTeamId(EOutbreakTeam::Neutral);

	TestTrue(TEXT("Same team friendly"), OutbreakTeam::GetAttitude(Player, Player) == ETeamAttitude::Friendly);
	TestTrue(TEXT("Different teams hostile"), OutbreakTeam::GetAttitude(Player, Hostile) == ETeamAttitude::Hostile);
	TestTrue(TEXT("Neutral is neutral"), OutbreakTeam::GetAttitude(Player, Neutral) == ETeamAttitude::Neutral);
	TestTrue(TEXT("Neutral either side"), OutbreakTeam::GetAttitude(Neutral, Hostile) == ETeamAttitude::Neutral);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakTeammateControllerTest, "Outbreak.AI.Controller.OrdersAndCompletion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 * The squad AI controller turns orders into behaviours and completes them: Move becomes Hold on
 * arrival, Regroup completes near the player, Attack completes when the target dies. Without an
 * order, the teammate follows when far from its slot. (Navigation itself is checked in the editor.)
 */
bool FOutbreakTeammateControllerTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerState* PlayerState = TestWorld.Spawn<AOutbreakPlayerState>();
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakTeammateCharacter* Teammate = TestWorld.Spawn<AOutbreakTeammateCharacter>(FVector(500.0f, 0.0f, 0.0f));
	AOutbreakCombatTestTarget* Target = TestWorld.Spawn<AOutbreakCombatTestTarget>(FVector(2000.0f, 0.0f, 0.0f));
	if (!PlayerState || !Player || !Teammate || !Target)
	{
		AddError(TEXT("Spawning failed"));
		return false;
	}

	if (!Teammate->GetController())
	{
		Teammate->SpawnDefaultController();
	}
	AOutbreakSquadAIController* Controller = Cast<AOutbreakSquadAIController>(Teammate->GetController());
	if (!TestNotNull(TEXT("Teammate has a squad AI controller"), Controller))
	{
		return false;
	}

	UOutbreakSquadComponent* Squad = PlayerState->GetSquadComponent();
	UOutbreakSquadMemberComponent* Member = Teammate->GetSquadMemberComponent();
	Squad->RegisterPlayerMember(Player->GetSquadMemberComponent());
	Squad->RegisterMember(Member);

	// Autonomous: 800 cm from the follow slot behind the player, so it follows.
	Controller->UpdateBehavior();
	TestTrue(TEXT("Far from slot: follows"), Controller->GetCurrentBehavior() == EOutbreakTeammateBehavior::FollowPlayer);
	TestTrue(TEXT("Behaviour replicated to the member"), Member->GetCurrentBehavior() == EOutbreakTeammateBehavior::FollowPlayer);

	Teammate->SetActorLocation(Controller->GetMoveLocation());
	Controller->UpdateBehavior();
	TestTrue(TEXT("At slot: idle"), Controller->GetCurrentBehavior() == EOutbreakTeammateBehavior::Idle);

	// Move: the controller reacts to the order immediately.
	const FVector Destination(0.0f, 1000.0f, Teammate->GetActorLocation().Z);
	Squad->AuthIssueCommand(FOutbreakSquadCommand::MakeMove(Destination), FOutbreakCommandRecipients::ForSquad());
	TestTrue(TEXT("Move order: moving"), Controller->GetCurrentBehavior() == EOutbreakTeammateBehavior::MoveToLocation);
	TestTrue(TEXT("Move destination"), Controller->GetMoveLocation().Equals(Destination, 1.0));

	Teammate->SetActorLocation(Destination);
	Controller->UpdateBehavior();
	TestTrue(TEXT("Arrived: holds"), Controller->GetCurrentBehavior() == EOutbreakTeammateBehavior::HoldPosition);
	TestTrue(TEXT("Move became a Hold order"), Member->GetActiveCommandType() == EOutbreakSquadCommandType::Hold);

	// Regroup completes near the player.
	Squad->AuthIssueCommand(FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Regroup), FOutbreakCommandRecipients::ForSquad());
	TestTrue(TEXT("Regroup order"), Controller->GetCurrentBehavior() == EOutbreakTeammateBehavior::Regroup);
	Teammate->SetActorLocation(Controller->GetMoveLocation());
	Controller->UpdateBehavior();
	TestTrue(TEXT("Regroup completed"), Member->GetCommandStatus() == EOutbreakSquadCommandStatus::Completed);
	TestTrue(TEXT("Back to autonomous"), Member->GetActiveCommandType() == EOutbreakSquadCommandType::None);

	// Attack completes when the target dies.
	Squad->AuthIssueCommand(FOutbreakSquadCommand::MakeAttack(Target), FOutbreakCommandRecipients::ForSquad());
	TestTrue(TEXT("Attack order"), Controller->GetCurrentBehavior() == EOutbreakTeammateBehavior::AttackTarget);
	TestTrue(TEXT("Attack target is the combat target"), Controller->GetCombatTarget() == Target);
	Target->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	Controller->UpdateBehavior();
	TestTrue(TEXT("Attack completed"), Member->GetCommandStatus() == EOutbreakSquadCommandStatus::Completed);

	// A dead teammate does nothing.
	Teammate->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	Controller->UpdateBehavior();
	TestTrue(TEXT("Dead teammate idle"), Controller->GetCurrentBehavior() == EOutbreakTeammateBehavior::Idle);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
