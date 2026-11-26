#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/OutbreakSquadAIController.h"
#include "AI/OutbreakTeammateBehaviorTypes.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Engine/DamageEvents.h"
#include "Game/OutbreakPlayerState.h"
#include "Characters/OutbreakEnemyCharacter.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Player/OutbreakPlayerController.h"
#include "Player/OutbreakSquadCommandComponent.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadConfiguration.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakSquadSettings.h"
#include "Squad/OutbreakTeammateCharacter.h"
#include "Tests/OutbreakTestWorld.h"
#include "UObject/Package.h"

namespace OutbreakTacticalTests
{
	EOutbreakTeammateBehavior Select(const FOutbreakBehaviorContext& Context)
	{
		return OutbreakTeammateBehavior::SelectBehavior(Context, OutbreakTeammateBehavior::GetDefaultPriorityOrder()).Behavior;
	}

	struct FSquadFixture
	{
		AOutbreakPlayerState* PlayerState = nullptr;
		AOutbreakPlayerCharacter* Player = nullptr;
		AOutbreakTeammateCharacter* Teammate = nullptr;
		AOutbreakSquadAIController* Controller = nullptr;
		UOutbreakSquadComponent* Squad = nullptr;

		bool Create(FOutbreakTestWorld& TestWorld, const FVector& TeammateLocation)
		{
			PlayerState = TestWorld.Spawn<AOutbreakPlayerState>();
			Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
			Teammate = TestWorld.Spawn<AOutbreakTeammateCharacter>(TeammateLocation);
			if (!PlayerState || !Player || !Teammate)
			{
				return false;
			}
			if (!Teammate->GetController())
			{
				Teammate->SpawnDefaultController();
			}
			Controller = Cast<AOutbreakSquadAIController>(Teammate->GetController());
			Squad = PlayerState->GetSquadComponent();
			Squad->RegisterPlayerMember(Player->GetSquadMemberComponent());
			Squad->RegisterMember(Teammate->GetSquadMemberComponent());
			return Controller != nullptr;
		}
	};
}

using namespace OutbreakTacticalTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakTacticalBehaviorTest, "Outbreak.AI.Behavior.TacticalOrdersAndCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Defend, Cover and Retreat map to behaviours; under fire with known cover, threat response takes cover. */
bool FOutbreakTacticalBehaviorTest::RunTest(const FString& Parameters)
{
	FOutbreakBehaviorContext Context;
	Context.bHasPlayer = true;

	Context.ActiveCommand = EOutbreakSquadCommandType::Defend;
	TestTrue(TEXT("Defend"), Select(Context) == EOutbreakTeammateBehavior::DefendPosition);
	Context.ActiveCommand = EOutbreakSquadCommandType::Cover;
	TestTrue(TEXT("Cover"), Select(Context) == EOutbreakTeammateBehavior::TakeCover);
	Context.ActiveCommand = EOutbreakSquadCommandType::Retreat;
	TestTrue(TEXT("Retreat"), Select(Context) == EOutbreakTeammateBehavior::Retreat);

	Context.ActiveCommand = EOutbreakSquadCommandType::None;
	Context.bHasThreat = true;
	TestTrue(TEXT("Threat, not under fire: engage"), Select(Context) == EOutbreakTeammateBehavior::EngageThreat);
	Context.bUnderFire = true;
	TestTrue(TEXT("Under fire, no cover known: engage"), Select(Context) == EOutbreakTeammateBehavior::EngageThreat);
	Context.bHasCover = true;
	TestTrue(TEXT("Under fire with cover: take cover"), Select(Context) == EOutbreakTeammateBehavior::TakeCover);

	Context.ActiveCommand = EOutbreakSquadCommandType::Hold;
	TestTrue(TEXT("An explicit order still wins over autonomous cover"), Select(Context) == EOutbreakTeammateBehavior::HoldPosition);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakTacticalOrdersSizesTest, "Outbreak.Squad.Commands.TacticalOrdersAllSizes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Phase 7 acceptance: Defend, Cover and Retreat are accepted by every teammate for every allowed squad size. */
bool FOutbreakTacticalOrdersSizesTest::RunTest(const FString& Parameters)
{
	const FOutbreakSquadSizeLimits& Limits = GetDefault<UOutbreakSquadSettings>()->SizeLimits;
	const EOutbreakSquadCommandType Orders[] = {
		EOutbreakSquadCommandType::Defend, EOutbreakSquadCommandType::Cover, EOutbreakSquadCommandType::Retreat };

	for (int32 SquadSize = Limits.MinSquadSize; SquadSize <= Limits.MaxSquadSize; ++SquadSize)
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerState* PlayerState = TestWorld.Spawn<AOutbreakPlayerState>();
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		UOutbreakSquadConfiguration* Configuration = NewObject<UOutbreakSquadConfiguration>(GetTransientPackage());
		Configuration->SquadSize = SquadSize;
		Configuration->InitialFormation = EOutbreakFormation::Line;

		UOutbreakSquadComponent* Squad = PlayerState->GetSquadComponent();
		if (!TestTrue(FString::Printf(TEXT("Squad of %d"), SquadSize), Squad->InitializeSquad(Configuration, Player)))
		{
			continue;
		}
		TestTrue(TEXT("Initial formation from configuration"), Squad->GetFormation() == EOutbreakFormation::Line);

		for (const EOutbreakSquadCommandType Order : Orders)
		{
			int32 Accepted = 0;
			for (const FOutbreakCommandResult& Result : Squad->AuthIssueCommand(FOutbreakSquadCommand::Make(Order), FOutbreakCommandRecipients::ForSquad()))
			{
				Accepted += Result.WasAccepted() ? 1 : 0;
			}
			TestEqual(FString::Printf(TEXT("Squad of %d: order %d accepted by all"), SquadSize, static_cast<int32>(Order)), Accepted, SquadSize - 1);
		}

		// Every teammate's controller turns the last order (Retreat) into the Retreat behaviour.
		for (const UOutbreakSquadMemberComponent* Member : Squad->GetTeammates())
		{
			const APawn* TeammatePawn = Cast<APawn>(Member->GetOwner());
			const AOutbreakSquadAIController* Controller = TeammatePawn ? Cast<AOutbreakSquadAIController>(TeammatePawn->GetController()) : nullptr;
			TestTrue(FString::Printf(TEXT("Squad of %d: teammate retreating"), SquadSize),
				Controller && Controller->GetCurrentBehavior() == EOutbreakTeammateBehavior::Retreat);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakDefendCoverRetreatControllerTest, "Outbreak.AI.Controller.DefendCoverRetreat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakDefendCoverRetreatControllerTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	FSquadFixture Fixture;
	if (!TestTrue(TEXT("Fixture"), Fixture.Create(TestWorld, FVector(-400.0f, 0.0f, 0.0f))))
	{
		return false;
	}
	UOutbreakSquadMemberComponent* Member = Fixture.Teammate->GetSquadMemberComponent();

	// Defend a point: the teammate goes to a slot around it, not onto it.
	const FVector DefendPoint(2000.0f, 0.0f, 0.0f);
	FOutbreakSquadCommand Defend = FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Defend);
	Defend.TargetLocation = DefendPoint;
	Defend.bHasTargetLocation = true;
	Fixture.Squad->AuthIssueCommand(Defend, FOutbreakCommandRecipients::ForSquad());
	TestTrue(TEXT("Defending"), Fixture.Controller->GetCurrentBehavior() == EOutbreakTeammateBehavior::DefendPosition);
	const float SlotDistance = FVector::Dist(Fixture.Controller->GetMoveLocation(), DefendPoint);
	TestTrue(TEXT("Defend slot near the point"), SlotDistance > 10.0f && SlotDistance <= 800.0f);
	TestTrue(TEXT("Cover anchor is the defended point"), [&]()
	{
		FVector Anchor;
		return Fixture.Controller->GetCoverAnchorLocation(Anchor) && Anchor.Equals(DefendPoint, 1.0);
	}());

	// Cover without a known threat or cover query: hold the ordered spot.
	Fixture.Squad->AuthIssueCommand(FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Cover), FOutbreakCommandRecipients::ForSquad());
	TestTrue(TEXT("Taking cover"), Fixture.Controller->GetCurrentBehavior() == EOutbreakTeammateBehavior::TakeCover);
	TestTrue(TEXT("No cover found: holds the ordered position"),
		Fixture.Controller->GetMoveLocation().Equals(Member->GetCurrentCommand().TargetLocation, 1.0));

	// Being shot marks the teammate as under fire and remembers where the shot came from.
	AOutbreakEnemyCharacter* Shooter = TestWorld.Spawn<AOutbreakEnemyCharacter>(FVector(-400.0f, 2000.0f, 0.0f));
	if (TestNotNull(TEXT("Shooter"), Shooter))
	{
		FDamageEvent DamageEvent;
		Fixture.Teammate->TakeDamage(5.0f, DamageEvent, nullptr, Shooter);
		TestTrue(TEXT("Under fire"), Fixture.Controller->IsUnderFire());
		FVector ThreatLocation;
		TestTrue(TEXT("Threat location remembered"),
			Fixture.Controller->GetCoverThreatLocation(ThreatLocation) && ThreatLocation.Equals(Shooter->GetActorLocation(), 1.0));
	}

	// Retreat moves away from that threat and completes on arrival.
	Fixture.Squad->AuthIssueCommand(FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Retreat), FOutbreakCommandRecipients::ForSquad());
	TestTrue(TEXT("Retreating"), Fixture.Controller->GetCurrentBehavior() == EOutbreakTeammateBehavior::Retreat);
	const FVector RetreatTo = Fixture.Controller->GetMoveLocation();
	if (Shooter)
	{
		TestTrue(TEXT("Retreat increases distance to the threat"),
			FVector::Dist(RetreatTo, Shooter->GetActorLocation()) > FVector::Dist(Fixture.Teammate->GetActorLocation(), Shooter->GetActorLocation()) + 500.0f);
	}
	Fixture.Teammate->SetActorLocation(RetreatTo);
	Fixture.Controller->UpdateBehavior();
	TestTrue(TEXT("Retreat completed"), Member->GetCommandStatus() == EOutbreakSquadCommandStatus::Completed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakFormationSwitchingTest, "Outbreak.Squad.Formation.Switching",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Switching formation moves teammates' follow slots. */
bool FOutbreakFormationSwitchingTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	FSquadFixture Fixture;
	if (!TestTrue(TEXT("Fixture"), Fixture.Create(TestWorld, FVector(0.0f, 2000.0f, 0.0f))))
	{
		return false;
	}

	Fixture.Squad->SetFormation(EOutbreakFormation::Column);
	Fixture.Controller->UpdateBehavior();
	const FVector ColumnSlot = Fixture.Controller->GetMoveLocation();
	TestTrue(TEXT("Column slot directly behind"), ColumnSlot.X < 0.0f && FMath::Abs(ColumnSlot.Y) < 1.0f);

	Fixture.Squad->SetFormation(EOutbreakFormation::Line);
	Fixture.Controller->UpdateBehavior();
	TestFalse(TEXT("Line slot differs"), Fixture.Controller->GetMoveLocation().Equals(ColumnSlot, 10.0));

	Fixture.Squad->SetFormation(EOutbreakFormation::Loose);
	Fixture.Squad->CycleFormation();
	TestTrue(TEXT("Cycling wraps to the first formation"), Fixture.Squad->GetFormation() == EOutbreakFormation::Column);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadMenuTest, "Outbreak.Squad.Commands.Menu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakSquadMenuTest::RunTest(const FString& Parameters)
{
	const TArray<FOutbreakSquadMenuEntry> Entries = UOutbreakSquadCommandComponent::GetMenuEntries();
	TestEqual(TEXT("Eight orders, the stealth toggle and five formations"), Entries.Num(), 14);
	TestTrue(TEXT("First entry is Follow"), !Entries[0].bIsFormation && Entries[0].Command == EOutbreakSquadCommandType::Follow);
	TestTrue(TEXT("Last entry is a formation"), Entries.Last().bIsFormation);

	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerController* PlayerController = TestWorld.Spawn<AOutbreakPlayerController>();
	UOutbreakSquadCommandComponent* Commands = PlayerController ? PlayerController->GetSquadCommandComponent() : nullptr;
	if (!TestNotNull(TEXT("Command component"), Commands))
	{
		return false;
	}

	TestTrue(TEXT("Closed: no menu text"), Commands->BuildMenuText().IsEmpty());
	Commands->ToggleMenu();
	TestTrue(TEXT("Opened"), Commands->IsMenuOpen());
	TestFalse(TEXT("Menu text shown"), Commands->BuildMenuText().IsEmpty());

	Commands->MenuPrevious();
	TestEqual(TEXT("Previous wraps to the last entry"), Commands->GetMenuIndex(), Entries.Num() - 1);
	Commands->MenuNext();
	Commands->MenuNext();
	TestEqual(TEXT("Next wraps to the second entry"), Commands->GetMenuIndex(), 1);

	TestFalse(TEXT("Confirm without a squad does nothing"), Commands->MenuConfirm());
	TestFalse(TEXT("Confirm closes the menu"), Commands->IsMenuOpen());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
