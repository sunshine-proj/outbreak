#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/OutbreakEnemyDefinition.h"
#include "AI/OutbreakEnemyTactics.h"
#include "Characters/OutbreakEnemyCharacter.h"
#include "Combat/OutbreakCombatTestTarget.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/OutbreakGameUserSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/OutbreakTestWorld.h"
#include "UObject/Package.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakFireBehavior.h"
#include "Weapons/OutbreakProjectile.h"
#include "Weapons/OutbreakWeaponDefinition.h"

/**
 * Phase 15 QA coverage: the gaps found by auditing the existing tests against ROADMAP 15.5
 * (Docs/Phase15-EditorSetup.md lists the full coverage matrix).
 */
namespace OutbreakQATests
{
	FOutbreakShotRequest MakeRequest(UWorld* World, const FVector& Origin, const TArray<FVector>& Directions, float Range, float Damage)
	{
		FOutbreakShotRequest Request;
		Request.World = World;
		Request.Origin = Origin;
		Request.Directions = Directions;
		Request.Range = Range;
		Request.ImpactDamage.Damage = Damage;
		return Request;
	}

	int32 CountProjectiles(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<AOutbreakProjectile> It(World); It; ++It)
		{
			++Count;
		}
		return Count;
	}
}

using namespace OutbreakQATests;

// --- 15.5 Test all weapon modes ------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakFireBehaviorDeliveryTest, "Outbreak.Weapons.FireBehaviors.Delivery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 * Hitscan, projectile and melee behaviours deliver one result per direction: an impact per trace
 * (at the end of range on a miss), a projectile per pellet. Hits damage the target when scene
 * queries are live in the test world.
 */
bool FOutbreakFireBehaviorDeliveryTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	UWorld* World = TestWorld.Get();
	const FVector Origin(0.0f, 0.0f, 100.0f);
	const TArray<FVector> Forward = { FVector::ForwardVector };
	const TArray<FVector> Sideways = { FVector::RightVector, -FVector::RightVector };

	// Misses into empty space: deterministic, no scene needed.
	{
		const UOutbreakHitscanFireBehavior* Hitscan = GetDefault<UOutbreakHitscanFireBehavior>();
		TArray<FOutbreakShotImpact> Impacts;
		Hitscan->ExecuteShot(MakeRequest(World, Origin, Sideways, 3000.0f, 10.0f), Impacts);
		TestEqual(TEXT("Hitscan: one impact per direction"), Impacts.Num(), 2);
		TestFalse(TEXT("Hitscan miss is not a blocking hit"), Impacts[0].bBlockingHit);
		TestTrue(TEXT("Hitscan miss ends at range"), Impacts[0].Location.Equals(Origin + FVector::RightVector * 3000.0f, 1.0f));

		const UOutbreakMeleeFireBehavior* Melee = GetDefault<UOutbreakMeleeFireBehavior>();
		Impacts.Reset();
		Melee->ExecuteShot(MakeRequest(World, Origin, Sideways, 180.0f, 10.0f), Impacts);
		TestEqual(TEXT("Melee: one impact per direction"), Impacts.Num(), 2);
		TestTrue(TEXT("Melee miss ends at its short reach"), Impacts[1].Location.Equals(Origin - FVector::RightVector * 180.0f, 1.0f));
	}

	// Projectiles: one per pellet, flying along its direction; none without a projectile class.
	{
		UOutbreakProjectileFireBehavior* Projectile = NewObject<UOutbreakProjectileFireBehavior>(GetTransientPackage());
		TArray<FOutbreakShotImpact> Impacts;
		const int32 Before = CountProjectiles(World);
		Projectile->ExecuteShot(MakeRequest(World, Origin, Sideways, 5000.0f, 10.0f), Impacts);
		TestEqual(TEXT("Projectile: one actor per direction"), CountProjectiles(World) - Before, 2);
		TestEqual(TEXT("Projectile impacts come later, from the projectile"), Impacts.Num(), 0);
		for (TActorIterator<AOutbreakProjectile> It(World); It; ++It)
		{
			const float Alignment = FMath::Abs(FVector::DotProduct(It->GetActorForwardVector(), FVector::RightVector));
			TestTrue(TEXT("Projectile faces its direction"), Alignment > 0.99f);
		}

		Projectile->ProjectileClass = nullptr;
		AddExpectedError(TEXT("no world or ProjectileClass"), EAutomationExpectedErrorFlags::Contains, 1);
		const int32 Count = CountProjectiles(World);
		Projectile->ExecuteShot(MakeRequest(World, Origin, Forward, 5000.0f, 10.0f), Impacts);
		TestEqual(TEXT("No projectile class: nothing spawned"), CountProjectiles(World), Count);

		// Out of the way of the traces below, which start at the same origin.
		for (TActorIterator<AOutbreakProjectile> It(World); It; ++It)
		{
			It->Destroy();
		}
	}

	// Hits: a target in front. Scene queries are not live in every test world (see Outbreak.Stealth.Noise.Occlusion),
	// so damage is checked only when the trace reports a hit.
	{
		AOutbreakCombatTestTarget* Target = TestWorld.Spawn<AOutbreakCombatTestTarget>(FVector(150.0f, 0.0f, 100.0f));
		UOutbreakHealthComponent* Health = Target->GetHealthComponent();
		const float Full = Health->GetHealth();

		TArray<FOutbreakShotImpact> Impacts;
		GetDefault<UOutbreakMeleeFireBehavior>()->ExecuteShot(MakeRequest(World, Origin, Forward, 180.0f, 25.0f), Impacts);
		if (Impacts.Num() == 1 && Impacts[0].bBlockingHit)
		{
			TestEqual(TEXT("Melee hit within reach damages"), static_cast<double>(Health->GetHealth()), static_cast<double>(Full - 25.0f), 0.01);
		}
		else
		{
			AddInfo(TEXT("Scene queries not live in this test world: melee hit damage not checked here (verify in the QA map)."));
		}

		const float AfterMelee = Health->GetHealth();
		Impacts.Reset();
		GetDefault<UOutbreakMeleeFireBehavior>()->ExecuteShot(MakeRequest(World, Origin, Forward, 50.0f, 25.0f), Impacts);
		TestEqual(TEXT("Melee out of reach does no damage"), static_cast<double>(Health->GetHealth()), static_cast<double>(AfterMelee), 0.01);

		Impacts.Reset();
		GetDefault<UOutbreakHitscanFireBehavior>()->ExecuteShot(MakeRequest(World, Origin, Forward, 3000.0f, 10.0f), Impacts);
		if (Impacts.Num() == 1 && Impacts[0].bBlockingHit)
		{
			TestEqual(TEXT("Hitscan hit damages"), static_cast<double>(Health->GetHealth()), static_cast<double>(AfterMelee - 10.0f), 0.01);
			TestTrue(TEXT("Hitscan stops at the target"), Impacts[0].Location.X < 200.0f);
		}
	}
	return true;
}

// --- 15.5 Test all enemy archetypes (Phase 14 additions) -----------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakPhase14ArchetypesTest, "Outbreak.Enemy.Archetypes.SentinelAndSwarmer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 * The Sentinel and Swarmer described in Docs/Phase14-EditorSetup.md are valid with existing data
 * only, and behave differently: the sentinel covers and holds its perch, the swarmer charges.
 */
bool FOutbreakPhase14ArchetypesTest::RunTest(const FString& Parameters)
{
	UOutbreakAmmoTypeDefinition* Ammo = NewObject<UOutbreakAmmoTypeDefinition>(GetTransientPackage());
	auto MakeWeapon = [Ammo](UOutbreakFireBehavior* Behavior, float Range, int32 Magazine, bool bUnlimited)
	{
		UOutbreakWeaponDefinition* Weapon = NewObject<UOutbreakWeaponDefinition>(GetTransientPackage());
		Weapon->MagazineCapacity = Magazine;
		Weapon->bUnlimitedAmmo = bUnlimited;
		FOutbreakFireModeDefinition& Mode = Weapon->FireModes.AddDefaulted_GetRef();
		Mode.FireBehavior = Behavior;
		Mode.Range = Range;
		Mode.bAutomatic = false;
		Weapon->CompatibleAmmoTypes.Add(Ammo);
		return Weapon;
	};

	UOutbreakEnemyDefinition* Sentinel = NewObject<UOutbreakEnemyDefinition>(GetTransientPackage());
	Sentinel->MaxHealth = 120.0f;
	Sentinel->SightRadius = 6000.0f;
	Sentinel->LoseSightRadius = 7000.0f;
	Sentinel->PeripheralVisionHalfAngle = 30.0f;
	Sentinel->SightDetectionRate = 0.8f;
	Sentinel->HearingSensitivity = 0.6f;
	Sentinel->MinCombatRange = 1500.0f;
	Sentinel->PreferredCombatRange = 3500.0f;
	Sentinel->MaxCombatRange = 6000.0f;
	Sentinel->Tactics.CombatStyle = EOutbreakEnemyCombatStyle::Ranged;
	Sentinel->Tactics.bPreferCover = true;
	Sentinel->Tactics.bCanFlank = false;
	Sentinel->Tactics.LeashDistance = 400.0f;
	Sentinel->Tactics.RetreatHealthFraction = 0.0f;
	Sentinel->Weapon = MakeWeapon(NewObject<UOutbreakHitscanFireBehavior>(GetTransientPackage()), 7000.0f, 4, false);

	UOutbreakEnemyDefinition* Swarmer = NewObject<UOutbreakEnemyDefinition>(GetTransientPackage());
	Swarmer->MaxHealth = 35.0f;
	Swarmer->MovementSettings.WalkSpeed = 450.0f;
	Swarmer->MovementSettings.SprintSpeed = 850.0f;
	Swarmer->SightRadius = 1800.0f;
	Swarmer->LoseSightRadius = 2200.0f;
	Swarmer->HearingSensitivity = 1.5f;
	Swarmer->MinCombatRange = 0.0f;
	Swarmer->PreferredCombatRange = 100.0f;
	Swarmer->MaxCombatRange = 180.0f;
	Swarmer->Tactics.CombatStyle = EOutbreakEnemyCombatStyle::Melee;
	Swarmer->Tactics.bCanFlank = false;
	Swarmer->Tactics.MaxFlankersPerTarget = 0;
	Swarmer->Tactics.RetreatHealthFraction = 0.0f;
	Swarmer->Weapon = MakeWeapon(NewObject<UOutbreakMeleeFireBehavior>(GetTransientPackage()), 180.0f, 1, true);
	Swarmer->AmmoReserve = 0;

	for (const UOutbreakEnemyDefinition* Definition : { Sentinel, Swarmer })
	{
		TArray<FText> Errors;
		TestTrue(FString::Printf(TEXT("%s definition valid"), Definition == Sentinel ? TEXT("Sentinel") : TEXT("Swarmer")), Definition->Validate(Errors));
		for (const FText& Error : Errors)
		{
			AddError(Error.ToString());
		}
	}

	FOutbreakTestWorld TestWorld;
	auto Spawn = [&TestWorld](UOutbreakEnemyDefinition* Definition, const FVector& Location)
	{
		const FTransform Transform(FRotator::ZeroRotator, Location);
		AOutbreakEnemyCharacter* Enemy = TestWorld.Get()->SpawnActorDeferred<AOutbreakEnemyCharacter>(
			AOutbreakEnemyCharacter::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Enemy)
		{
			Enemy->SetEnemyDefinition(Definition);
			Enemy->FinishSpawning(Transform);
		}
		return Enemy;
	};
	AOutbreakEnemyCharacter* SwarmerEnemy = Spawn(Swarmer, FVector::ZeroVector);
	AOutbreakEnemyCharacter* SentinelEnemy = Spawn(Sentinel, FVector(0.0f, 1000.0f, 0.0f));
	if (!TestNotNull(TEXT("Swarmer spawned"), SwarmerEnemy) || !TestNotNull(TEXT("Sentinel spawned"), SentinelEnemy))
	{
		return false;
	}
	TestEqual(TEXT("Swarmer is fragile"), static_cast<double>(SwarmerEnemy->GetHealthComponent()->GetMaxHealth()), 35.0, 0.01);
	TestEqual(TEXT("Swarmer is fast"), static_cast<double>(SwarmerEnemy->GetCharacterMovement()->MaxWalkSpeed), 450.0, 0.01);
	TestEqual(TEXT("Sentinel health"), static_cast<double>(SentinelEnemy->GetHealthComponent()->GetMaxHealth()), 120.0, 0.01);

	FOutbreakEnemyCombatContext Context;
	Context.bTargetVisible = true;
	Context.bHasCover = true;
	Context.bUnderFire = true;
	Context.Style = Sentinel->Tactics.CombatStyle;
	Context.bPreferCover = Sentinel->Tactics.bPreferCover;
	TestTrue(TEXT("Sentinel takes cover under fire"), OutbreakEnemyTactics::SelectCombatMove(Context) == EOutbreakEnemyCombatMove::TakeCover);
	Context.Style = Swarmer->Tactics.CombatStyle;
	Context.bPreferCover = false;
	TestTrue(TEXT("Swarmer charges under fire"), OutbreakEnemyTactics::SelectCombatMove(Context) == EOutbreakEnemyCombatMove::Charge);

	const FVector Home(0.0f, 1000.0f, 0.0f);
	const FVector FarAway = Home + FVector(3000.0f, 0.0f, 0.0f);
	const FVector Held = OutbreakEnemyTactics::ClampToLeash(FarAway, Home, Sentinel->Tactics.LeashDistance);
	TestTrue(TEXT("Sentinel combat positions stay on its perch"), FVector::Dist2D(Held, Home) <= Sentinel->Tactics.LeashDistance + 1.0f);
	return true;
}

// --- Volume settings -----------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakVolumeSettingsTest, "Outbreak.Campaign.Settings.Volume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Volumes clamp to 0..1; the master volume scales every channel. */
bool FOutbreakVolumeSettingsTest::RunTest(const FString& Parameters)
{
	using Settings = UOutbreakGameUserSettings;
	using EV = EOutbreakVolumeChannel;
	TestEqual(TEXT("Clamp low"), static_cast<double>(Settings::ClampVolume(-1.0f)), 0.0, 0.0001);
	TestEqual(TEXT("Clamp high"), static_cast<double>(Settings::ClampVolume(3.0f)), 1.0, 0.0001);
	TestEqual(TEXT("NaN = full volume"), static_cast<double>(Settings::ClampVolume(std::numeric_limits<float>::quiet_NaN())), 1.0, 0.0001);
	TestEqual(TEXT("Master class gets master"), static_cast<double>(Settings::ComputeClassVolume(0.5f, 0.2f, EV::Master)), 0.5, 0.0001);
	TestEqual(TEXT("Channel = master x channel"), static_cast<double>(Settings::ComputeClassVolume(0.5f, 0.8f, EV::Voice)), 0.4, 0.0001);
	TestEqual(TEXT("Muted master mutes everything"), static_cast<double>(Settings::ComputeClassVolume(0.0f, 1.0f, EV::Music)), 0.0, 0.0001);

	UOutbreakGameUserSettings* Object = NewObject<UOutbreakGameUserSettings>(GetTransientPackage());
	Object->SetVolume(EV::Effects, 0.3f);
	Object->SetVolume(EV::Music, 7.0f);
	TestEqual(TEXT("Effects stored"), static_cast<double>(Object->GetVolume(EV::Effects)), 0.3, 0.0001);
	TestEqual(TEXT("Music clamped"), static_cast<double>(Object->GetVolume(EV::Music)), 1.0, 0.0001);
	TestEqual(TEXT("Others untouched"), static_cast<double>(Object->GetVolume(EV::Voice)), 1.0, 0.0001);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
