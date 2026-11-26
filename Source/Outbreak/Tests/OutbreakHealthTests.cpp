#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/OutbreakCombatTestTarget.h"
#include "Combat/OutbreakDamageTypes.h"
#include "Combat/OutbreakHealthComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Tests/OutbreakTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakHealthDamageTest, "Outbreak.Combat.Health.DamageAndDeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Damage through the engine pipeline reduces health; reaching zero kills; the dead take no damage. */
bool FOutbreakHealthDamageTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakCombatTestTarget* Target = TestWorld.Spawn<AOutbreakCombatTestTarget>();
	if (!TestNotNull(TEXT("Target spawned"), Target))
	{
		return false;
	}

	UOutbreakHealthComponent* Health = Target->GetHealthComponent();
	TestEqual(TEXT("Starts at max health"), Health->GetHealth(), Health->GetMaxHealth());
	TestTrue(TEXT("Starts alive"), Health->IsAlive());

	UGameplayStatics::ApplyDamage(Target, 30.0f, nullptr, nullptr, UDamageType::StaticClass());
	TestEqual(TEXT("Damage reduces health"), Health->GetHealth(), Health->GetMaxHealth() - 30.0f);
	TestTrue(TEXT("Still alive"), Health->IsAlive());

	UGameplayStatics::ApplyDamage(Target, 1000.0f, nullptr, nullptr, UDamageType::StaticClass());
	TestEqual(TEXT("Health clamps at zero"), Health->GetHealth(), 0.0f);
	TestTrue(TEXT("Dead"), Health->GetHealthState() == EOutbreakHealthState::Dead);
	TestTrue(TEXT("Dead target hidden"), Target->IsHidden());
	TestFalse(TEXT("Dead target has no collision"), Target->GetActorEnableCollision());

	TestEqual(TEXT("Dead take no further damage"), Health->ApplyDamage(10.0f, nullptr), 0.0f);
	TestEqual(TEXT("Negative damage ignored"), Health->ApplyDamage(-10.0f, nullptr), 0.0f);

	Health->ResetHealth();
	TestTrue(TEXT("Reset revives"), Health->IsAlive());
	TestEqual(TEXT("Reset restores max health"), Health->GetHealth(), Health->GetMaxHealth());
	TestFalse(TEXT("Revived target visible"), Target->IsHidden());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakHealthIncapacitationTest, "Outbreak.Combat.Health.Incapacitation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakHealthIncapacitationTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakCombatTestTarget* Target = TestWorld.Spawn<AOutbreakCombatTestTarget>();
	if (!TestNotNull(TEXT("Target spawned"), Target))
	{
		return false;
	}

	UOutbreakHealthComponent* Health = Target->GetHealthComponent();
	Health->SetCanBeIncapacitated(true);

	Health->ApplyDamage(Health->GetMaxHealth(), nullptr);
	TestTrue(TEXT("Zero health incapacitates"), Health->GetHealthState() == EOutbreakHealthState::Incapacitated);

	Health->ApplyDamage(1.0f, nullptr);
	TestTrue(TEXT("Damage while incapacitated kills"), Health->GetHealthState() == EOutbreakHealthState::Dead);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakHealthDamageTypesTest, "Outbreak.Combat.Health.DamageTypeMultipliers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakHealthDamageTypesTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakCombatTestTarget* Target = TestWorld.Spawn<AOutbreakCombatTestTarget>();
	if (!TestNotNull(TEXT("Target spawned"), Target))
	{
		return false;
	}

	UOutbreakHealthComponent* Health = Target->GetHealthComponent();
	const UDamageType* Ballistic = GetDefault<UOutbreakBallisticDamageType>();
	const UDamageType* Explosive = GetDefault<UOutbreakExplosiveDamageType>();

	TestEqual(TEXT("No multiplier by default"), Health->CalculateDamage(40.0f, Explosive), 40.0f);

	Health->SetDamageMultiplier(UOutbreakExplosiveDamageType::StaticClass(), 0.5f);
	TestEqual(TEXT("Explosive resisted"), Health->CalculateDamage(40.0f, Explosive), 20.0f);
	TestEqual(TEXT("Ballistic unaffected"), Health->CalculateDamage(40.0f, Ballistic), 40.0f);

	Health->SetDamageMultiplier(UOutbreakDamageType::StaticClass(), 2.0f);
	TestEqual(TEXT("Parent-class multiplier applies to subclasses"), Health->CalculateDamage(40.0f, Ballistic), 80.0f);
	TestEqual(TEXT("Matching multipliers stack"), Health->CalculateDamage(40.0f, Explosive), 40.0f);

	Health->SetDamageMultiplier(UOutbreakExplosiveDamageType::StaticClass(), 0.0f);
	TestEqual(TEXT("Replacing an entry, immunity"), Health->CalculateDamage(40.0f, Explosive), 0.0f);
	TestEqual(TEXT("Immune damage changes nothing"), Health->ApplyDamage(40.0f, Explosive), 0.0f);
	TestEqual(TEXT("Health unchanged"), Health->GetHealth(), Health->GetMaxHealth());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakTeamTest, "Outbreak.Combat.Teams",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** The combat test target is hostile by default; the player is on the player team; both have health. */
bool FOutbreakTeamTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakCombatTestTarget* Target = TestWorld.Spawn<AOutbreakCombatTestTarget>();
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>(FVector(500.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("Target spawned"), Target) || !TestNotNull(TEXT("Player spawned"), Player))
	{
		return false;
	}

	TestTrue(TEXT("Target is hostile"), Target->GetGenericTeamId() == OutbreakTeam::ToGenericTeamId(EOutbreakTeam::Hostile));
	TestTrue(TEXT("Player is on the player team"), Player->GetGenericTeamId() == OutbreakTeam::ToGenericTeamId(EOutbreakTeam::Player));
	TestTrue(TEXT("Teams differ"), Target->GetGenericTeamId() != Player->GetGenericTeamId());
	TestNotNull(TEXT("Player has health"), Player->GetHealthComponent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakPlayerDeathTest, "Outbreak.Combat.PlayerDeathDisablesWeapons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakPlayerDeathTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	if (!TestNotNull(TEXT("Player spawned"), Player))
	{
		return false;
	}

	TestTrue(TEXT("Weapons enabled while alive"), Player->GetWeaponComponent()->AreWeaponsEnabled());

	Player->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	TestFalse(TEXT("Player is dead"), Player->GetHealthComponent()->IsAlive());
	TestFalse(TEXT("Death disables weapons"), Player->GetWeaponComponent()->AreWeaponsEnabled());
	TestFalse(TEXT("Dead player cannot sprint"), Player->IsSprinting());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
