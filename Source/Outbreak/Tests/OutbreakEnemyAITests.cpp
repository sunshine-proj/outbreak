#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/OutbreakEnemyAIController.h"
#include "AI/OutbreakEnemyDefinition.h"
#include "AI/OutbreakEnemyTypes.h"
#include "AI/OutbreakPatrolRoute.h"
#include "Characters/OutbreakEnemyCharacter.h"
#include "Combat/OutbreakCombatTestTarget.h"
#include "Combat/OutbreakDamageTypes.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Combat/OutbreakTeam.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Squad/OutbreakTeammateCharacter.h"
#include "Tests/OutbreakTestWorld.h"
#include "UObject/Package.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakFireBehavior.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

namespace OutbreakEnemyAITests
{
	UOutbreakEnemyDefinition* MakeDefinition()
	{
		UOutbreakEnemyDefinition* Definition = NewObject<UOutbreakEnemyDefinition>(GetTransientPackage());
		Definition->MaxHealth = 150.0f;
		Definition->MovementSettings.WalkSpeed = 200.0f;
		Definition->MovementSettings.SprintSpeed = 450.0f;
		Definition->PatrolWaitTime = 0.0f;
		return Definition;
	}

	/** Spawns an enemy with Definition applied during initialization, and makes sure it is possessed. */
	AOutbreakEnemyCharacter* SpawnEnemy(FOutbreakTestWorld& TestWorld, UOutbreakEnemyDefinition* Definition,
		const FVector& Location, AOutbreakPatrolRoute* Route = nullptr)
	{
		const FTransform Transform(FRotator::ZeroRotator, Location);
		AOutbreakEnemyCharacter* Enemy = TestWorld.Get()->SpawnActorDeferred<AOutbreakEnemyCharacter>(
			AOutbreakEnemyCharacter::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Enemy)
		{
			return nullptr;
		}
		Enemy->SetEnemyDefinition(Definition);
		Enemy->SetPatrolRoute(Route);
		Enemy->FinishSpawning(Transform);
		if (!Enemy->GetController())
		{
			Enemy->SpawnDefaultController();
		}
		return Enemy;
	}
}

using namespace OutbreakEnemyAITests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnemyStateRulesTest, "Outbreak.Enemy.Behavior.StateSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Phase 8 awareness states: Idle / Patrol < Suspicious < Investigating < Combat, then Searching after losing a target. */
bool FOutbreakEnemyStateRulesTest::RunTest(const FString& Parameters)
{
	FOutbreakEnemyStateContext Context;
	Context.SuspiciousThreshold = 0.2f;
	Context.InvestigateThreshold = 0.5f;
	Context.LoseTargetTime = 3.0f;
	TestTrue(TEXT("No route, nothing sensed: idle"), OutbreakEnemyBehavior::SelectState(Context) == EOutbreakEnemyState::Idle);

	Context.bHasPatrolRoute = true;
	TestTrue(TEXT("With a route: patrol"), OutbreakEnemyBehavior::SelectState(Context) == EOutbreakEnemyState::Patrol);

	Context.Suspicion = 0.3f;
	Context.bHasStimulusLocation = true;
	TestTrue(TEXT("Mild suspicion: stop and look"), OutbreakEnemyBehavior::SelectState(Context) == EOutbreakEnemyState::Suspicious);

	Context.Suspicion = 0.6f;
	TestTrue(TEXT("Strong suspicion: investigate"), OutbreakEnemyBehavior::SelectState(Context) == EOutbreakEnemyState::Investigating);

	Context.bHasStimulusLocation = false;
	TestTrue(TEXT("Nowhere to go: only suspicious"), OutbreakEnemyBehavior::SelectState(Context) == EOutbreakEnemyState::Suspicious);

	Context.bHasDetectedTarget = true;
	Context.TimeSinceTargetSeen = 1.0f;
	TestTrue(TEXT("Detected target: combat"), OutbreakEnemyBehavior::SelectState(Context) == EOutbreakEnemyState::Combat);

	Context.TimeSinceTargetSeen = 5.0f;
	Context.bSearching = true;
	TestTrue(TEXT("Lost for too long: search"), OutbreakEnemyBehavior::SelectState(Context) == EOutbreakEnemyState::Searching);

	Context.bHasDetectedTarget = false;
	Context.bSearching = false;
	Context.Suspicion = 0.0f;
	TestTrue(TEXT("Search over, calm again: patrol"), OutbreakEnemyBehavior::SelectState(Context) == EOutbreakEnemyState::Patrol);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnemyTargetingTest, "Outbreak.Enemy.Targeting.Prioritization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakEnemyTargetingTest::RunTest(const FString& Parameters)
{
	FOutbreakTargetScoreWeights Weights;
	Weights.DistancePenaltyPerMeter = 1.0f;
	Weights.RecentAttackerBonus = 20.0f;
	Weights.PlayerBonus = 5.0f;

	const float Near = OutbreakEnemyTargeting::ScoreTarget(1000.0f, false, false, Weights);
	const float Far = OutbreakEnemyTargeting::ScoreTarget(3000.0f, false, false, Weights);
	TestTrue(TEXT("Nearer target preferred"), Near > Far);

	const float FarAttacker = OutbreakEnemyTargeting::ScoreTarget(2500.0f, true, false, Weights);
	TestTrue(TEXT("A recent attacker outranks a slightly nearer target"), FarAttacker > Near);

	const float NearTeammate = OutbreakEnemyTargeting::ScoreTarget(1000.0f, false, false, Weights);
	const float SlightlyFartherPlayer = OutbreakEnemyTargeting::ScoreTarget(1300.0f, false, true, Weights);
	TestTrue(TEXT("Player bias outweighs a few metres"), SlightlyFartherPlayer > NearTeammate);

	const float MuchFartherPlayer = OutbreakEnemyTargeting::ScoreTarget(2000.0f, false, true, Weights);
	TestTrue(TEXT("Player bias does not outweigh a large distance"), NearTeammate > MuchFartherPlayer);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnemyCombatPositionTest, "Outbreak.Enemy.Tactics.CombatPositioning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakEnemyCombatPositionTest::RunTest(const FString& Parameters)
{
	const FVector Target = FVector::ZeroVector;
	const float Min = 500.0f, Preferred = 1200.0f, Max = 2500.0f;

	const FVector InBand(1500.0f, 0.0f, 0.0f);
	TestTrue(TEXT("Inside the band: hold"),
		OutbreakEnemyTactics::ComputeCombatPosition(InBand, Target, true, Min, Preferred, Max).Equals(InBand));

	const FVector TooFar(4000.0f, 0.0f, 0.0f);
	TestTrue(TEXT("Too far: close to preferred range"),
		OutbreakEnemyTactics::ComputeCombatPosition(TooFar, Target, true, Min, Preferred, Max).Equals(FVector(1200.0f, 0.0f, 0.0f), 1.0));

	const FVector TooClose(200.0f, 0.0f, 0.0f);
	TestTrue(TEXT("Too close: back off to preferred range"),
		OutbreakEnemyTactics::ComputeCombatPosition(TooClose, Target, true, Min, Preferred, Max).Equals(FVector(1200.0f, 0.0f, 0.0f), 1.0));

	TestTrue(TEXT("Not visible: close in to minimum range"),
		OutbreakEnemyTactics::ComputeCombatPosition(InBand, Target, false, Min, Preferred, Max).Equals(FVector(500.0f, 0.0f, 0.0f), 1.0));

	TestTrue(TEXT("On top of the target: hold"),
		OutbreakEnemyTactics::ComputeCombatPosition(Target, Target, true, Min, Preferred, Max).Equals(Target));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakPatrolRouteTest, "Outbreak.Enemy.Patrol.RouteOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakPatrolRouteTest::RunTest(const FString& Parameters)
{
	int32 Direction = 1;
	TArray<int32> Loop;
	int32 Index = 0;
	for (int32 Step = 0; Step < 5; ++Step)
	{
		Index = AOutbreakPatrolRoute::GetNextIndex(Index, 3, true, Direction);
		Loop.Add(Index);
	}
	TestTrue(TEXT("Looping route wraps"), Loop == TArray<int32>({ 1, 2, 0, 1, 2 }));

	Direction = 1;
	Index = 0;
	TArray<int32> PingPong;
	for (int32 Step = 0; Step < 6; ++Step)
	{
		Index = AOutbreakPatrolRoute::GetNextIndex(Index, 3, false, Direction);
		PingPong.Add(Index);
	}
	TestTrue(TEXT("Non-looping route goes back and forth"), PingPong == TArray<int32>({ 1, 2, 1, 0, 1, 2 }));

	TestEqual(TEXT("Single-point route stays"), AOutbreakPatrolRoute::GetNextIndex(0, 1, true, Direction), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnemyDefinitionTest, "Outbreak.Enemy.Definition.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakEnemyDefinitionTest::RunTest(const FString& Parameters)
{
	TArray<FText> Errors;
	TestTrue(TEXT("Defaults are valid"), MakeDefinition()->Validate(Errors));

	UOutbreakEnemyDefinition* BadRanges = MakeDefinition();
	BadRanges->MinCombatRange = 2000.0f;
	Errors.Reset();
	TestFalse(TEXT("Min above preferred rejected"), BadRanges->Validate(Errors));

	UOutbreakEnemyDefinition* BadSight = MakeDefinition();
	BadSight->LoseSightRadius = 100.0f;
	Errors.Reset();
	TestFalse(TEXT("Lose-sight radius below sight radius rejected"), BadSight->Validate(Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnemyConfigurationTest, "Outbreak.Enemy.Character.ConfiguredByDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Health, resistances, movement and weapon come from the definition; the enemy is hostile to the squad. */
bool FOutbreakEnemyConfigurationTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;

	UOutbreakAmmoTypeDefinition* Ammo = NewObject<UOutbreakAmmoTypeDefinition>(GetTransientPackage());
	UOutbreakWeaponDefinition* Claws = NewObject<UOutbreakWeaponDefinition>(GetTransientPackage());
	FOutbreakFireModeDefinition& Mode = Claws->FireModes.AddDefaulted_GetRef();
	Mode.FireBehavior = NewObject<UOutbreakHitscanFireBehavior>(Claws);
	Claws->CompatibleAmmoTypes.Add(Ammo);

	UOutbreakEnemyDefinition* Definition = MakeDefinition();
	Definition->Weapon = Claws;
	Definition->AmmoReserve = 77;
	FOutbreakDamageMultiplier& Resistance = Definition->DamageMultipliers.AddDefaulted_GetRef();
	Resistance.DamageType = UOutbreakEnergyDamageType::StaticClass();
	Resistance.Multiplier = 0.5f;

	AOutbreakEnemyCharacter* Enemy = SpawnEnemy(TestWorld, Definition, FVector(1000.0f, 0.0f, 0.0f));
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakTeammateCharacter* Teammate = TestWorld.Spawn<AOutbreakTeammateCharacter>(FVector(-300.0f, 0.0f, 0.0f));
	AOutbreakCombatTestTarget* HostileTarget = TestWorld.Spawn<AOutbreakCombatTestTarget>(FVector(0.0f, 1000.0f, 0.0f));
	if (!TestNotNull(TEXT("Enemy spawned"), Enemy) || !Player || !Teammate || !HostileTarget)
	{
		return false;
	}

	TestTrue(TEXT("Enemy AI controller"), Cast<AOutbreakEnemyAIController>(Enemy->GetController()) != nullptr);
	TestEqual(TEXT("Max health from definition"), Enemy->GetHealthComponent()->GetMaxHealth(), 150.0f);
	TestEqual(TEXT("Starts at full health"), Enemy->GetHealthComponent()->GetHealth(), 150.0f);
	TestEqual(TEXT("Energy resistance applied"),
		Enemy->GetHealthComponent()->CalculateDamage(40.0f, GetDefault<UOutbreakEnergyDamageType>()), 20.0f);
	TestEqual(TEXT("Walk speed from definition"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 200.0f);

	TestTrue(TEXT("Hostile to the player"), OutbreakTeam::IsHostile(Enemy, Player));
	TestTrue(TEXT("Hostile to teammates"), OutbreakTeam::IsHostile(Enemy, Teammate));
	TestFalse(TEXT("Not hostile to other hostiles"), OutbreakTeam::IsHostile(Enemy, HostileTarget));

	// BeginPlay (weapon hand-out) does not run in the test world; give it as BeginPlay would.
	Enemy->DispatchBeginPlay();
	TestTrue(TEXT("Weapon from definition"), Enemy->GetWeaponComponent()->GetActiveWeaponDefinition() == Claws);
	TestEqual(TEXT("Ammunition reserve from definition"), Enemy->GetWeaponComponent()->GetReserveAmmo(Ammo), 77);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnemyControllerTest, "Outbreak.Enemy.Controller.PatrolAlertAndDeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Patrol advances along the route, a strong stimulus (noise, unseen damage) makes it investigate, death stops the enemy. */
bool FOutbreakEnemyControllerTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPatrolRoute* Route = TestWorld.Spawn<AOutbreakPatrolRoute>();
	if (!Route)
	{
		return false;
	}
	Route->SetPoints({ FVector(0.0f, 0.0f, 0.0f), FVector(1000.0f, 0.0f, 0.0f) }, true);

	UOutbreakEnemyDefinition* Definition = MakeDefinition();
	Definition->CorpseLifetime = 10.0f;
	AOutbreakEnemyCharacter* Enemy = SpawnEnemy(TestWorld, Definition, FVector::ZeroVector, Route);
	AOutbreakEnemyAIController* Controller = Enemy ? Cast<AOutbreakEnemyAIController>(Enemy->GetController()) : nullptr;
	if (!TestNotNull(TEXT("Enemy controller"), Controller))
	{
		return false;
	}

	// Standing on point 0 with no wait time: patrol moves on to point 1.
	Controller->UpdateBehavior();
	TestTrue(TEXT("Patrolling"), Controller->GetCurrentState() == EOutbreakEnemyState::Patrol);
	TestTrue(TEXT("State replicated on the character"), Enemy->GetEnemyState() == EOutbreakEnemyState::Patrol);
	TestEqual(TEXT("Advanced to point 1"), Controller->GetPatrolIndex(), 1);
	TestTrue(TEXT("Heading to point 1"), Controller->GetMoveLocation().Equals(Route->GetWorldPoint(1), 1.0));
	TestFalse(TEXT("Walking on patrol"), Enemy->IsSprinting());

	Enemy->SetActorLocation(Route->GetWorldPoint(1));
	Controller->UpdateBehavior();
	TestEqual(TEXT("Loops back to point 0"), Controller->GetPatrolIndex(), 0);

	// A loud nearby noise (or a hit from an unseen attacker) makes it investigate at once.
	const FVector Noise(500.0f, 800.0f, 0.0f);
	Controller->ReportAlert(Noise);
	Controller->UpdateBehavior();
	TestTrue(TEXT("Investigating"), Controller->GetCurrentState() == EOutbreakEnemyState::Investigating);
	TestTrue(TEXT("Goes to the noise"), Controller->GetMoveLocation().Equals(Noise, 1.0));
	TestTrue(TEXT("Hurries when fully alarmed"), Enemy->IsSprinting());

	// Damage from a hostile it cannot see also raises an alert towards the attacker.
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>(FVector(-2000.0f, 0.0f, 0.0f));
	if (Player)
	{
		FDamageEvent DamageEvent;
		Enemy->TakeDamage(10.0f, DamageEvent, nullptr, Player);
		TestTrue(TEXT("Stimulus moved towards the attacker"), Controller->GetStimulusLocation().Equals(Player->GetActorLocation(), 1.0));
		TestEqual(TEXT("Damage applied"), Enemy->GetHealthComponent()->GetHealth(), 140.0f);
	}

	// Death stops the enemy and starts the corpse timer.
	Enemy->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	TestTrue(TEXT("Dead"), Enemy->IsDead());
	TestTrue(TEXT("Corpse does not block"), Enemy->GetCapsuleComponent()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestTrue(TEXT("Corpse removed later"), Enemy->GetLifeSpan() > 0.0f);
	TestNull(TEXT("No combat target when dead"), Controller->GetCombatTarget());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
