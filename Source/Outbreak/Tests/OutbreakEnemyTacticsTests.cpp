#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/OutbreakEnemyAIController.h"
#include "AI/OutbreakEnemyCoordinationSubsystem.h"
#include "AI/OutbreakEnemyDefinition.h"
#include "AI/OutbreakEnemyTactics.h"
#include "AI/OutbreakEnemyTypes.h"
#include "Characters/OutbreakEnemyCharacter.h"
#include "Combat/OutbreakCombatTestTarget.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Tests/OutbreakTestWorld.h"
#include "UObject/Package.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakFireBehavior.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

namespace OutbreakEnemyTacticsTests
{
	UOutbreakEnemyDefinition* MakeDefinition()
	{
		UOutbreakEnemyDefinition* Definition = NewObject<UOutbreakEnemyDefinition>(GetTransientPackage());
		Definition->MaxHealth = 100.0f;
		Definition->PatrolWaitTime = 0.0f;
		return Definition;
	}

	AOutbreakEnemyCharacter* SpawnEnemy(FOutbreakTestWorld& TestWorld, UOutbreakEnemyDefinition* Definition, const FVector& Location)
	{
		const FTransform Transform(FRotator::ZeroRotator, Location);
		AOutbreakEnemyCharacter* Enemy = TestWorld.Get()->SpawnActorDeferred<AOutbreakEnemyCharacter>(
			AOutbreakEnemyCharacter::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Enemy)
		{
			return nullptr;
		}
		Enemy->SetEnemyDefinition(Definition);
		Enemy->FinishSpawning(Transform);
		if (!Enemy->GetController())
		{
			Enemy->SpawnDefaultController();
		}
		return Enemy;
	}

	AOutbreakEnemyAIController* GetController(AOutbreakEnemyCharacter* Enemy)
	{
		return Enemy ? Cast<AOutbreakEnemyAIController>(Enemy->GetController()) : nullptr;
	}

	/** A weapon with one fire mode using Behavior. */
	UOutbreakWeaponDefinition* MakeWeapon(UOutbreakFireBehavior* Behavior, UOutbreakWeaponDefinition* Weapon,
		float Range, int32 MagazineCapacity)
	{
		Weapon->MagazineCapacity = MagazineCapacity;
		FOutbreakFireModeDefinition& Mode = Weapon->FireModes.AddDefaulted_GetRef();
		Mode.FireBehavior = Behavior;
		Mode.Range = Range;
		Mode.bAutomatic = false;
		Weapon->CompatibleAmmoTypes.Add(NewObject<UOutbreakAmmoTypeDefinition>(GetTransientPackage()));
		return Weapon;
	}
}

using namespace OutbreakEnemyTacticsTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnemyCombatMoveTest, "Outbreak.Enemy.Tactics.CombatMoveSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Retreat > Charge > Flank > TakeCover > Engage, and where each combat style stands. */
bool FOutbreakEnemyCombatMoveTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakEnemyTactics;

	FOutbreakEnemyCombatContext Context;
	TestTrue(TEXT("Default: engage"), SelectCombatMove(Context) == EOutbreakEnemyCombatMove::Engage);

	Context.bHasCover = true;
	TestTrue(TEXT("Cover known but not needed: engage"), SelectCombatMove(Context) == EOutbreakEnemyCombatMove::Engage);
	Context.bUnderFire = true;
	TestTrue(TEXT("Under fire with cover: take cover"), SelectCombatMove(Context) == EOutbreakEnemyCombatMove::TakeCover);
	Context.bUnderFire = false;
	Context.bReloading = true;
	TestTrue(TEXT("Reloading with cover: take cover"), SelectCombatMove(Context) == EOutbreakEnemyCombatMove::TakeCover);
	Context.bReloading = false;
	Context.bPreferCover = true;
	TestTrue(TEXT("Prefers cover: take cover"), SelectCombatMove(Context) == EOutbreakEnemyCombatMove::TakeCover);

	Context.bFlankRole = true;
	TestTrue(TEXT("Flank role outranks cover"), SelectCombatMove(Context) == EOutbreakEnemyCombatMove::Flank);
	Context.bFlankReached = true;
	TestTrue(TEXT("Flank made: back to cover"), SelectCombatMove(Context) == EOutbreakEnemyCombatMove::TakeCover);

	Context.Style = EOutbreakEnemyCombatStyle::Advance;
	TestTrue(TEXT("Advance style ignores cover"), SelectCombatMove(Context) == EOutbreakEnemyCombatMove::Engage);

	Context.Style = EOutbreakEnemyCombatStyle::Melee;
	Context.bFlankReached = false;
	TestTrue(TEXT("Melee charges"), SelectCombatMove(Context) == EOutbreakEnemyCombatMove::Charge);

	Context.bRetreating = true;
	TestTrue(TEXT("Retreat outranks everything"), SelectCombatMove(Context) == EOutbreakEnemyCombatMove::Retreat);

	// Positions: self at 2000 cm from the target, ranges 500 / 1200 / 2500.
	const FVector Self(2000.0f, 0.0f, 0.0f);
	const FVector Target(0.0f, 0.0f, 0.0f);
	TestTrue(TEXT("Ranged holds inside the band"),
		ComputeEngagePosition(EOutbreakEnemyCombatStyle::Ranged, Self, Target, true, 500.0f, 1200.0f, 2500.0f).Equals(Self, 1.0));
	TestTrue(TEXT("Advance closes to the preferred range"),
		ComputeEngagePosition(EOutbreakEnemyCombatStyle::Advance, Self, Target, true, 500.0f, 1200.0f, 2500.0f).Equals(FVector(1200.0f, 0.0f, 0.0f), 1.0));
	TestTrue(TEXT("Advance does not back off when close"),
		ComputeEngagePosition(EOutbreakEnemyCombatStyle::Advance, FVector(300.0f, 0.0f, 0.0f), Target, true, 500.0f, 1200.0f, 2500.0f)
			.Equals(FVector(300.0f, 0.0f, 0.0f), 1.0));
	TestTrue(TEXT("Melee runs at the target"),
		ComputeEngagePosition(EOutbreakEnemyCombatStyle::Melee, Self, Target, true, 0.0f, 100.0f, 200.0f).Equals(Target, 1.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnemyFlankingTest, "Outbreak.Enemy.Tactics.Flanking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Flank positions, flank completion, squad-aware side choice and role assignment. */
bool FOutbreakEnemyFlankingTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakEnemyTactics;

	const FVector Target(0.0f, 0.0f, 0.0f);
	const FVector Pin(1000.0f, 0.0f, 0.0f);

	const FVector Left = ComputeFlankPosition(Target, Pin, 90.0f, 1, 1000.0f);
	const FVector Right = ComputeFlankPosition(Target, Pin, 90.0f, -1, 1000.0f);
	TestTrue(TEXT("Left flank at 90 degrees"), Left.Equals(FVector(0.0f, 1000.0f, 0.0f), 1.0));
	TestTrue(TEXT("Right flank on the other side"), Right.Equals(FVector(0.0f, -1000.0f, 0.0f), 1.0));

	TestFalse(TEXT("Next to the pin: flank not made"), HasFlanked(Target, Pin, FVector(900.0f, 100.0f, 0.0f), 90.0f));
	TestTrue(TEXT("Round the side: flank made"), HasFlanked(Target, Pin, FVector(100.0f, 900.0f, 0.0f), 90.0f));

	// Squad-aware: another squad member stands near the left flank position.
	const TArray<FVector> Squad = { FVector(100.0f, 900.0f, 0.0f) };
	TestEqual(TEXT("Picks the side away from the squad"), ChooseFlankSide(Left, Right, Squad), -1);
	TestEqual(TEXT("No other squad members: left"), ChooseFlankSide(Left, Right, TArray<FVector>()), 1);

	// Roles.
	using namespace OutbreakEnemyCoordination;
	FOutbreakEngagementCandidate Near; Near.DistanceToTarget = 800.0f; Near.bCanFlank = true;
	FOutbreakEngagementCandidate Mid; Mid.DistanceToTarget = 1200.0f; Mid.bCanFlank = true;
	FOutbreakEngagementCandidate Far; Far.DistanceToTarget = 1600.0f; Far.bCanFlank = true;

	TArray<int32> Roles = AssignFlankRoles(TArray<FOutbreakEngagementCandidate>{ Near }, 2);
	TestEqual(TEXT("A lone enemy pins"), Roles[0], static_cast<int32>(INDEX_NONE));

	Roles = AssignFlankRoles(TArray<FOutbreakEngagementCandidate>{ Far, Near, Mid }, 2);
	TestEqual(TEXT("Nearest pins"), Roles[1], static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("Next nearest flanks first"), Roles[2], 0);
	TestEqual(TEXT("Farthest flanks second"), Roles[0], 1);

	Roles = AssignFlankRoles(TArray<FOutbreakEngagementCandidate>{ Far, Near, Mid }, 1);
	TestEqual(TEXT("Flanker limit: one flanker"), Roles[2], 0);
	TestEqual(TEXT("Flanker limit: others engage"), Roles[0], static_cast<int32>(INDEX_NONE));

	FOutbreakEngagementCandidate Heavy; Heavy.DistanceToTarget = 1500.0f; Heavy.bCanFlank = false;
	Roles = AssignFlankRoles(TArray<FOutbreakEngagementCandidate>{ Near, Heavy }, 2);
	TestEqual(TEXT("A non-flanker pins even when farther"), Roles[1], static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("The flanker flanks"), Roles[0], 0);

	FOutbreakEngagementCandidate FarWasFlanking = Far; FarWasFlanking.bWasFlanking = true;
	Roles = AssignFlankRoles(TArray<FOutbreakEngagementCandidate>{ Near, Mid, FarWasFlanking }, 1);
	TestEqual(TEXT("A current flanker keeps its role"), Roles[2], 0);
	TestEqual(TEXT("A nearer newcomer does not take it"), Roles[1], static_cast<int32>(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnemyTargetSwitchingTest, "Outbreak.Enemy.Tactics.TargetSwitching",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Shared-target penalty spreads fire; the switch margin keeps the current target. */
bool FOutbreakEnemyTargetSwitchingTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakEnemyCoordination;

	FOutbreakTargetScoreWeights Weights;
	FOutbreakTargetCandidate Alone;
	Alone.DistanceCm = 1000.0f;
	FOutbreakTargetCandidate Crowded = Alone;
	Crowded.OtherAttackers = 2;

	TestEqual(TEXT("No allies on it: Phase 5 score"), static_cast<double>(ScoreCandidate(Alone, Weights, 4.0f)), -10.0, 0.01);
	TestEqual(TEXT("Two allies on it: penalised"), static_cast<double>(ScoreCandidate(Crowded, Weights, 4.0f)), -18.0, 0.01);

	// Squad-aware: a slightly farther, unengaged squad member beats a nearer one two allies are on.
	FOutbreakTargetCandidate FartherAlone;
	FartherAlone.DistanceCm = 1500.0f;
	TestTrue(TEXT("Fire spreads to the unengaged target"),
		ScoreCandidate(FartherAlone, Weights, 4.0f) > ScoreCandidate(Crowded, Weights, 4.0f));

	const TArray<float> Scores = { -10.0f, -7.0f };
	TestEqual(TEXT("No current target: best"), SelectTarget(Scores, INDEX_NONE, 5.0f), 1);
	TestEqual(TEXT("Small advantage: keeps current"), SelectTarget(Scores, 0, 5.0f), 0);
	TestEqual(TEXT("Large advantage: switches"), SelectTarget(TArray<float>{ -10.0f, -2.0f }, 0, 5.0f), 1);
	TestEqual(TEXT("Current target is best: keeps it"), SelectTarget(Scores, 1, 5.0f), 1);
	TestEqual(TEXT("No candidates"), SelectTarget(TArray<float>(), INDEX_NONE, 5.0f), static_cast<int32>(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnemyRetreatRulesTest, "Outbreak.Enemy.Tactics.RetreatAndLeash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Retreat triggers, retreat position, leash clamping, and the Retreating state. */
bool FOutbreakEnemyRetreatRulesTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakEnemyTactics;

	TestTrue(TEXT("Low health: retreat"), ShouldRetreatForHealth(0.2f, 0.25f, false));
	TestFalse(TEXT("Healthy: no retreat"), ShouldRetreatForHealth(0.6f, 0.25f, false));
	TestFalse(TEXT("Only once for health"), ShouldRetreatForHealth(0.2f, 0.25f, true));
	TestFalse(TEXT("Fraction 0 = never"), ShouldRetreatForHealth(0.1f, 0.0f, false));
	TestFalse(TEXT("Dead is not retreating"), ShouldRetreatForHealth(0.0f, 0.25f, false));

	TestTrue(TEXT("4 hostiles vs 2 at ratio 2: outnumbered"), IsOutnumbered(4, 2, 2.0f));
	TestFalse(TEXT("3 hostiles vs 2 at ratio 2: not"), IsOutnumbered(3, 2, 2.0f));
	TestFalse(TEXT("Ratio 0 = never"), IsOutnumbered(10, 1, 0.0f));
	TestFalse(TEXT("No hostiles: never"), IsOutnumbered(0, 1, 0.5f));

	TestTrue(TEXT("Retreats straight away from the threat"),
		ComputeRetreatPosition(FVector(100.0f, 0.0f, 0.0f), FVector::ZeroVector, 1000.0f).Equals(FVector(1100.0f, 0.0f, 0.0f), 1.0));

	const FVector Home(0.0f, 0.0f, 50.0f);
	TestTrue(TEXT("Inside the leash: unchanged"),
		ClampToLeash(FVector(300.0f, 0.0f, 0.0f), Home, 500.0f).Equals(FVector(300.0f, 0.0f, 0.0f), 1.0));
	TestTrue(TEXT("Beyond the leash: pulled back to its edge"),
		ClampToLeash(FVector(0.0f, 2000.0f, 0.0f), Home, 500.0f).Equals(FVector(0.0f, 500.0f, 0.0f), 1.0));
	TestTrue(TEXT("Leash 0 = unlimited"), ClampToLeash(FVector(9000.0f, 0.0f, 0.0f), Home, 0.0f).Equals(FVector(9000.0f, 0.0f, 0.0f), 1.0));
	TestFalse(TEXT("A target lost beyond the leash is not searched for"), IsWithinLeash(FVector(2000.0f, 0.0f, 0.0f), Home, 500.0f));

	FOutbreakEnemyStateContext StateContext;
	StateContext.bHasDetectedTarget = true;
	StateContext.bRetreating = true;
	TestTrue(TEXT("Retreating outranks combat"),
		OutbreakEnemyBehavior::SelectState(StateContext) == EOutbreakEnemyState::Retreating);

	TArray<FText> Errors;
	FOutbreakEnemyTacticsSettings Tactics;
	Tactics.Validate(Errors);
	TestEqual(TEXT("Default tactics valid"), Errors.Num(), 0);
	Tactics.FlankAngle = 5.0f;
	Tactics.Validate(Errors);
	TestTrue(TEXT("Invalid flank angle rejected"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnemyCoordinationTest, "Outbreak.Enemy.Coordination.SharedSightingsAndRoles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Engagements, shared sightings, pin/flank roles and the coordination radius. */
bool FOutbreakEnemyCoordinationTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	UOutbreakEnemyCoordinationSubsystem* Coordination = TestWorld.Get()->GetSubsystem<UOutbreakEnemyCoordinationSubsystem>();
	UOutbreakEnemyDefinition* Definition = MakeDefinition();
	AOutbreakEnemyAIController* A = GetController(SpawnEnemy(TestWorld, Definition, FVector(800.0f, 0.0f, 0.0f)));
	AOutbreakEnemyAIController* B = GetController(SpawnEnemy(TestWorld, Definition, FVector(1200.0f, 200.0f, 0.0f)));
	AOutbreakEnemyAIController* Distant = GetController(SpawnEnemy(TestWorld, Definition, FVector(20000.0f, 0.0f, 0.0f)));
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	if (!TestNotNull(TEXT("Coordination subsystem"), Coordination) || !TestNotNull(TEXT("Enemy A"), A) || !B || !Distant || !Player)
	{
		return false;
	}

	const FVector PlayerLocation = Player->GetActorLocation();
	Coordination->ReportEngagement(A, Player, FVector(800.0f, 0.0f, 0.0f), true, true, PlayerLocation);
	Coordination->ReportEngagement(B, Player, FVector(1200.0f, 200.0f, 0.0f), true, false, FVector::ZeroVector);
	Coordination->ReportEngagement(Distant, Player, FVector(20000.0f, 0.0f, 0.0f), true, false, FVector::ZeroVector);

	TestTrue(TEXT("Engaged target recorded"), Coordination->GetEngagedTarget(A) == Player);
	TestEqual(TEXT("B counts A on the same target nearby"),
		Coordination->CountAlliesEngaging(Player, FVector(1200.0f, 200.0f, 0.0f), 2500.0f, B), 1);
	TestEqual(TEXT("Allies in combat near B"), Coordination->CountAlliesInCombat(FVector(1200.0f, 200.0f, 0.0f), 2500.0f, B), 1);

	FVector Seen;
	double SeenTime = 0.0;
	TestTrue(TEXT("B gets A's sighting"), Coordination->GetLatestSighting(Player, FVector(1200.0f, 200.0f, 0.0f), 2500.0f, B, Seen, SeenTime));
	TestTrue(TEXT("Sighting is where A saw the player"), Seen.Equals(PlayerLocation, 1.0));
	TestFalse(TEXT("The distant enemy is out of range"),
		Coordination->GetLatestSighting(Player, FVector(20000.0f, 0.0f, 0.0f), 2500.0f, Distant, Seen, SeenTime));

	const FOutbreakFlankAssignment RoleA = Coordination->GetFlankAssignment(A, 2500.0f, 2);
	const FOutbreakFlankAssignment RoleB = Coordination->GetFlankAssignment(B, 2500.0f, 2);
	TestFalse(TEXT("A (nearest) pins"), RoleA.IsFlanking());
	TestTrue(TEXT("B flanks"), RoleB.IsFlanking());
	TestTrue(TEXT("B flanks from A's line"), RoleB.PinLocation.Equals(FVector(800.0f, 0.0f, 0.0f), 1.0));
	TestFalse(TEXT("The distant enemy fights alone"), Coordination->GetFlankAssignment(Distant, 2500.0f, 2).IsFlanking());

	Coordination->RemoveEngagement(A);
	TestNull(TEXT("Removed"), Coordination->GetEngagedTarget(A));
	TestFalse(TEXT("Without a pin, B no longer flanks"), Coordination->GetFlankAssignment(B, 2500.0f, 2).IsFlanking());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnemyRetreatControllerTest, "Outbreak.Enemy.Controller.RetreatWhenHurt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** A badly hurt enemy retreats away from its attacker, inside its leash, and sprints. */
bool FOutbreakEnemyRetreatControllerTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	UOutbreakEnemyDefinition* Definition = MakeDefinition();
	Definition->Tactics.RetreatHealthFraction = 0.5f;
	Definition->Tactics.RetreatDistance = 1500.0f;
	Definition->Tactics.LeashDistance = 1000.0f;

	AOutbreakEnemyCharacter* Enemy = SpawnEnemy(TestWorld, Definition, FVector::ZeroVector);
	AOutbreakEnemyAIController* Controller = GetController(Enemy);
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>(FVector(-2000.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("Enemy controller"), Controller) || !Player)
	{
		return false;
	}

	// Shot by the (unseen) player, down to 40% health.
	FDamageEvent DamageEvent;
	Enemy->TakeDamage(10.0f, DamageEvent, nullptr, Player);
	Enemy->GetHealthComponent()->ApplyDamage(50.0f, nullptr);
	TestEqual(TEXT("Health at 40%"), Enemy->GetHealthComponent()->GetHealth(), 40.0f);

	Controller->UpdateBehavior();
	TestTrue(TEXT("Retreating"), Controller->GetCurrentState() == EOutbreakEnemyState::Retreating);
	TestTrue(TEXT("Retreat move"), Controller->GetCombatMove() == EOutbreakEnemyCombatMove::Retreat);
	TestTrue(TEXT("Away from the attacker"), Controller->GetRetreatLocation().X > 0.0);
	TestTrue(TEXT("Inside the leash"),
		FVector::Dist2D(Controller->GetRetreatLocation(), Controller->GetHomeLocation()) <= 1000.0 + 1.0);
	TestTrue(TEXT("Heading for the retreat position"), Controller->GetMoveLocation().Equals(Controller->GetRetreatLocation(), 1.0));
	TestTrue(TEXT("Sprints away"), Enemy->IsSprinting());
	TestTrue(TEXT("Counts as in combat for animation"), Enemy->IsInCombat());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnemyArchetypesTest, "Outbreak.Enemy.Archetypes.Configurations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Infantry, fast melee and heavy archetypes are pure data: valid, distinct, and the claws never run dry. */
bool FOutbreakEnemyArchetypesTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;

	// Alien infantry: ranged, cover, flanking, retreats when hurt.
	UOutbreakEnemyDefinition* Infantry = MakeDefinition();
	Infantry->Weapon = MakeWeapon(NewObject<UOutbreakHitscanFireBehavior>(GetTransientPackage()),
		NewObject<UOutbreakWeaponDefinition>(GetTransientPackage()), 10000.0f, 6);

	// Fast melee attacker: charges, never flanks or retreats, unlimited claws.
	UOutbreakEnemyDefinition* Melee = MakeDefinition();
	Melee->MaxHealth = 60.0f;
	Melee->MovementSettings.SprintSpeed = 750.0f;
	Melee->MinCombatRange = 0.0f;
	Melee->PreferredCombatRange = 100.0f;
	Melee->MaxCombatRange = 180.0f;
	Melee->Tactics.CombatStyle = EOutbreakEnemyCombatStyle::Melee;
	Melee->Tactics.RetreatHealthFraction = 0.0f;
	Melee->SightDetectionRate = 2.5f;
	UOutbreakWeaponDefinition* Claws = MakeWeapon(NewObject<UOutbreakMeleeFireBehavior>(GetTransientPackage()),
		NewObject<UOutbreakWeaponDefinition>(GetTransientPackage()), 200.0f, 1);
	Claws->bUnlimitedAmmo = true;
	Melee->Weapon = Claws;
	Melee->AmmoReserve = 0;

	// Heavy: slow, tough, advances without cover.
	UOutbreakEnemyDefinition* Heavy = MakeDefinition();
	Heavy->MaxHealth = 400.0f;
	Heavy->MovementSettings.WalkSpeed = 120.0f;
	Heavy->Tactics.CombatStyle = EOutbreakEnemyCombatStyle::Advance;
	Heavy->Tactics.bCanFlank = false;
	Heavy->Tactics.RetreatHealthFraction = 0.0f;

	for (const UOutbreakEnemyDefinition* Definition : { Infantry, Melee, Heavy })
	{
		TArray<FText> Errors;
		TestTrue(TEXT("Archetype definition valid"), Definition->Validate(Errors));
	}

	AOutbreakEnemyCharacter* MeleeEnemy = SpawnEnemy(TestWorld, Melee, FVector(0.0f, 0.0f, 0.0f));
	AOutbreakEnemyCharacter* HeavyEnemy = SpawnEnemy(TestWorld, Heavy, FVector(0.0f, 1000.0f, 0.0f));
	if (!TestNotNull(TEXT("Melee enemy"), MeleeEnemy) || !TestNotNull(TEXT("Heavy enemy"), HeavyEnemy))
	{
		return false;
	}
	TestEqual(TEXT("Heavy health"), HeavyEnemy->GetHealthComponent()->GetMaxHealth(), 400.0f);
	TestEqual(TEXT("Heavy walk speed"), HeavyEnemy->GetCharacterMovement()->MaxWalkSpeed, 120.0f);
	TestEqual(TEXT("Melee health"), MeleeEnemy->GetHealthComponent()->GetMaxHealth(), 60.0f);

	// Claws: a swing does not use up the magazine.
	MeleeEnemy->DispatchBeginPlay();
	UOutbreakWeaponComponent* Weapons = MeleeEnemy->GetWeaponComponent();
	TestTrue(TEXT("Claws equipped"), Weapons->GetActiveWeaponDefinition() == Claws);
	Weapons->StartFire();
	Weapons->StopFire();
	TestEqual(TEXT("Unlimited ammunition: magazine still full"), Weapons->GetAmmoInMagazine(), 1);
	TestTrue(TEXT("Can swing again"), Weapons->CanFire());

	// Different behaviour from the same inputs.
	FOutbreakEnemyCombatContext Context;
	Context.bHasCover = true;
	Context.bUnderFire = true;
	Context.Style = Infantry->Tactics.CombatStyle;
	TestTrue(TEXT("Infantry takes cover under fire"), OutbreakEnemyTactics::SelectCombatMove(Context) == EOutbreakEnemyCombatMove::TakeCover);
	Context.Style = Melee->Tactics.CombatStyle;
	TestTrue(TEXT("Melee charges under fire"), OutbreakEnemyTactics::SelectCombatMove(Context) == EOutbreakEnemyCombatMove::Charge);
	Context.Style = Heavy->Tactics.CombatStyle;
	TestTrue(TEXT("Heavy keeps advancing under fire"), OutbreakEnemyTactics::SelectCombatMove(Context) == EOutbreakEnemyCombatMove::Engage);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
