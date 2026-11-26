#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/OutbreakEnemyCharacter.h"
#include "Combat/OutbreakCombatTestTarget.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "Environment/OutbreakDoor.h"
#include "Environment/OutbreakEnvironmentSubsystem.h"
#include "Environment/OutbreakEnvironmentVolume.h"
#include "Environment/OutbreakSwitchable.h"
#include "Environment/OutbreakTimeOfDay.h"
#include "Interaction/OutbreakInteractable.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Inventory/OutbreakItemDefinition.h"
#include "Missions/OutbreakDestructibleObject.h"
#include "Missions/OutbreakMissionEquipment.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Stealth/OutbreakLightingSubsystem.h"
#include "Stealth/OutbreakStealthSettings.h"
#include "Tests/OutbreakTestWorld.h"
#include "UObject/Package.h"

namespace OutbreakEnvironmentTests
{
	UOutbreakItemDefinition* MakeKey()
	{
		UOutbreakItemDefinition* Key = NewObject<UOutbreakItemDefinition>(GetTransientPackage());
		Key->DisplayName = FText::FromString(TEXT("Keycard"));
		Key->Category = EOutbreakItemCategory::Mission;
		Key->Weight = 0.0f;
		Key->MaxStack = 1;
		return Key;
	}

	FString Prompt(AActor* Interactable, AActor* Instigator)
	{
		return IOutbreakInteractable::Execute_GetInteractionPrompt(Interactable, Instigator).ToString();
	}
}

using namespace OutbreakEnvironmentTests;

// --- Day / night ---------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakTimeOfDayCurveTest, "Outbreak.Environment.TimeOfDay.Curve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakTimeOfDayCurveTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakTimeOfDay;
	TestEqual(TEXT("Wraps past midnight"), WrapHour(25.0f), 1.0f, 0.001f);
	TestEqual(TEXT("Wraps negative hours"), WrapHour(-1.0f), 23.0f, 0.001f);

	TestEqual(TEXT("Noon: highest"), ComputeSunElevation(12.0f, 6.0f, 18.0f, 60.0f), 60.0f, 0.01f);
	TestEqual(TEXT("Sunrise: horizon"), ComputeSunElevation(6.0f, 6.0f, 18.0f, 60.0f), 0.0f, 0.01f);
	TestEqual(TEXT("Sunset: horizon"), ComputeSunElevation(18.0f, 6.0f, 18.0f, 60.0f), 0.0f, 0.01f);
	TestEqual(TEXT("Midnight: lowest"), ComputeSunElevation(0.0f, 6.0f, 18.0f, 60.0f), -60.0f, 0.01f);

	TestEqual(TEXT("Full daylight"), ComputeDaylight(60.0f), 1.0f, 0.001f);
	TestEqual(TEXT("Full night"), ComputeDaylight(-60.0f), 0.0f, 0.001f);
	TestEqual(TEXT("Twilight at the horizon"), ComputeDaylight(0.0f), 0.3164f, 0.001f);

	TestEqual(TEXT("Rises at its yaw"), ComputeSunYaw(6.0f, 6.0f, 18.0f, 90.0f), 90.0f, 0.01f);
	TestEqual(TEXT("Half way at noon"), ComputeSunYaw(12.0f, 6.0f, 18.0f, 90.0f), 180.0f, 0.01f);
	TestEqual(TEXT("Sets opposite"), ComputeSunYaw(18.0f, 6.0f, 18.0f, 90.0f), 270.0f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakTimeOfDayLightTest, "Outbreak.Environment.TimeOfDay.GameplayLight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** The hour moves the sun and changes the stealth light level: night is dark for enemies. */
bool FOutbreakTimeOfDayLightTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakTimeOfDay* Clock = TestWorld.Spawn<AOutbreakTimeOfDay>();
	ADirectionalLight* Sun = TestWorld.Spawn<ADirectionalLight>();
	UOutbreakLightingSubsystem* Lighting = TestWorld.Get()->GetSubsystem<UOutbreakLightingSubsystem>();
	if (!Clock || !Sun || !TestNotNull(TEXT("Lighting subsystem"), Lighting))
	{
		return false;
	}
	Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	Clock->SetLights(Sun, nullptr, nullptr);
	TestTrue(TEXT("Found by the lighting model"), AOutbreakTimeOfDay::Find(TestWorld.Get()) == Clock);

	const FVector OpenGround(0.0f, 0.0f, 100.0f);
	const float SunContribution = GetDefault<UOutbreakStealthSettings>()->DirectionalLightContribution;

	Clock->SetHour(0.0f);
	Lighting->InvalidateCache();
	TestEqual(TEXT("Midnight: sun off"), Sun->GetLightComponent()->Intensity, 0.0f, 0.001f);
	TestEqual(TEXT("Midnight: night ambient only"), Lighting->GetLightLevelAt(OpenGround), 0.03f, 0.001f);

	Clock->SetHour(12.0f);
	TestEqual(TEXT("Noon: full sun"), Sun->GetLightComponent()->Intensity, 10.0f, 0.001f);
	TestEqual(TEXT("Noon: sun high"), Sun->GetActorRotation().Pitch, -60.0, 0.1);
	TestEqual(TEXT("Noon: day ambient plus sunlight"), Lighting->GetLightLevelAt(OpenGround),
		FMath::Clamp(0.25f + SunContribution, 0.0f, 1.0f), 0.001f);

	Clock->SetHour(30.0f);
	TestEqual(TEXT("Hours wrap"), Clock->GetHour(), 6.0f, 0.001f);
	return true;
}

// --- Environment zones ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnvironmentVolumeTest, "Outbreak.Environment.Volume.PerceptionModifiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakEnvironmentVolumeTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakEnvironmentVolume* Forest = TestWorld.Spawn<AOutbreakEnvironmentVolume>(FVector::ZeroVector);
	AOutbreakEnvironmentVolume* Hall = TestWorld.Spawn<AOutbreakEnvironmentVolume>(FVector(1500.0f, 0.0f, 0.0f));
	UOutbreakEnvironmentSubsystem* Environment = TestWorld.Get()->GetSubsystem<UOutbreakEnvironmentSubsystem>();
	if (!Forest || !Hall || !TestNotNull(TEXT("Environment subsystem"), Environment))
	{
		return false;
	}

	FOutbreakEnvironmentPerception Fog;
	Fog.SightRangeMultiplier = 0.5f;
	Fog.FootstepLoudnessMultiplier = 0.6f;
	Forest->SetPerception(Fog);

	FOutbreakEnvironmentPerception Machinery;
	Machinery.HearingMultiplier = 0.4f;
	Machinery.FootstepLoudnessMultiplier = 1.5f;
	Hall->SetPerception(Machinery);
	Hall->SetPriority(1);
	Environment->InvalidateCache();

	TestEqual(TEXT("Forest: shorter sight"), Environment->GetPerceptionAt(FVector(-500.0f, 0.0f, 0.0f)).SightRangeMultiplier, 0.5f);
	TestEqual(TEXT("Forest: soft ground"), Environment->GetPerceptionAt(FVector(-500.0f, 0.0f, 0.0f)).FootstepLoudnessMultiplier, 0.6f);

	const FOutbreakEnvironmentPerception Overlap = Environment->GetPerceptionAt(FVector(700.0f, 0.0f, 0.0f));
	TestEqual(TEXT("Overlap: higher priority wins (hearing)"), Overlap.HearingMultiplier, 0.4f);
	TestEqual(TEXT("Overlap: higher priority wins (sight)"), Overlap.SightRangeMultiplier, 1.0f);

	const FOutbreakEnvironmentPerception Outside = Environment->GetPerceptionAt(FVector(9000.0f, 0.0f, 0.0f));
	TestTrue(TEXT("Outside every zone: neutral"), Outside.SightRangeMultiplier == 1.0f && Outside.HearingMultiplier == 1.0f
		&& Outside.FootstepLoudnessMultiplier == 1.0f);

	TestEqual(TEXT("Sight uses the worse of viewer and target zones"),
		Environment->GetSightRangeMultiplier(FVector(9000.0f, 0.0f, 0.0f), FVector(-500.0f, 0.0f, 0.0f)), 0.5f);
	return true;
}

// --- Doors ---------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakDoorTest, "Outbreak.Environment.Door.InteractLockAndNavigation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Doors open and close with Interact, lock, unlock with a key, open for AI, follow switches and gate AI paths. */
bool FOutbreakDoorTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakDoor* Door = TestWorld.Spawn<AOutbreakDoor>(FVector(200.0f, 0.0f, 0.0f));
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakEnemyCharacter* Enemy = TestWorld.Spawn<AOutbreakEnemyCharacter>(FVector(400.0f, 0.0f, 0.0f));
	if (!Door || !Player || !Enemy)
	{
		return false;
	}
	Enemy->SpawnDefaultController();

	TestFalse(TEXT("Starts closed"), Door->IsOpen());
	TestFalse(TEXT("Closed but AI-openable: AI may path through"), Door->BlocksNavigation());
	TestEqual(TEXT("Open prompt"), Prompt(Door, Player), FString(TEXT("Open door")));
	IOutbreakInteractable::Execute_Interact(Door, Player);
	TestTrue(TEXT("Interact opens"), Door->IsOpen());
	TestEqual(TEXT("Close prompt"), Prompt(Door, Player), FString(TEXT("Close door")));
	IOutbreakInteractable::Execute_Interact(Door, Player);
	TestFalse(TEXT("Interact closes"), Door->IsOpen());

	Door->SetLocked(true);
	TestTrue(TEXT("Locked: AI paths around"), Door->BlocksNavigation());
	TestEqual(TEXT("Locked prompt"), Prompt(Door, Player), FString(TEXT("Locked")));
	IOutbreakInteractable::Execute_Interact(Door, Player);
	TestFalse(TEXT("Locked door does not open"), Door->IsOpen());
	TestFalse(TEXT("AI cannot open a locked door"), Door->TryAutoOpen(Enemy));

	UOutbreakItemDefinition* Key = MakeKey();
	Door->SetKey(Key, /*bConsume=*/ true);
	TestEqual(TEXT("Names the key"), Prompt(Door, Player), FString(TEXT("Locked: requires Keycard")));
	Player->GetInventoryComponent()->AddItem(Key, 1);
	TestEqual(TEXT("Offers to unlock"), Prompt(Door, Player), FString(TEXT("Unlock with Keycard")));
	IOutbreakInteractable::Execute_Interact(Door, Player);
	TestTrue(TEXT("Key unlocks and opens"), !Door->IsLocked() && Door->IsOpen());
	TestEqual(TEXT("Key used up"), Player->GetInventoryComponent()->GetItemCount(Key), 0);

	Door->Close(Player);
	TestFalse(TEXT("Players do not auto-open doors"), Door->TryAutoOpen(Player));
	TestTrue(TEXT("AI walking in opens it"), Door->TryAutoOpen(Enemy) && Door->IsOpen());
	TestFalse(TEXT("Already open"), Door->TryAutoOpen(Enemy));

	IOutbreakSwitchable::Execute_SetSwitchedOn(Door, false, nullptr);
	TestTrue(TEXT("Switched off: closed and locked"), !Door->IsOpen() && Door->IsLocked());
	IOutbreakSwitchable::Execute_SetSwitchedOn(Door, true, nullptr);
	TestTrue(TEXT("Switched on: unlocked and open"), Door->IsOpen() && !Door->IsLocked());
	return true;
}

// --- Equipment -----------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEquipmentLinkTest, "Outbreak.Environment.Equipment.LinkedActorsAndToggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** A generator powers its lights and its security door, and can be switched off again. */
bool FOutbreakEquipmentLinkTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakMissionEquipment* Generator = TestWorld.Spawn<AOutbreakMissionEquipment>(FVector(100.0f, 0.0f, 0.0f));
	AOutbreakDoor* Door = TestWorld.Spawn<AOutbreakDoor>(FVector(800.0f, 0.0f, 0.0f));
	APointLight* Lamp = TestWorld.Spawn<APointLight>(FVector(400.0f, 0.0f, 300.0f));
	if (!Player || !Generator || !Door || !Lamp)
	{
		return false;
	}
	Door->SetLocked(true);
	Generator->SetLocked(false);
	Generator->SetCanToggle(true);
	Generator->SetLinkedActors({ Door, Lamp });

	TestFalse(TEXT("Generator off: lamp off"), Lamp->GetLightComponent()->IsVisible());

	IOutbreakInteractable::Execute_Interact(Generator, Player);
	TestTrue(TEXT("Switched on"), Generator->IsActivated());
	TestTrue(TEXT("Lamp on"), Lamp->GetLightComponent()->IsVisible());
	TestTrue(TEXT("Security door unlocked and open"), !Door->IsLocked() && Door->IsOpen());
	TestEqual(TEXT("Offers to switch off"), Prompt(Generator, Player), FString(TEXT("Switch off")));

	IOutbreakInteractable::Execute_Interact(Generator, Player);
	TestFalse(TEXT("Switched off"), Generator->IsActivated());
	TestFalse(TEXT("Lamp off again"), Lamp->GetLightComponent()->IsVisible());
	TestTrue(TEXT("Door closed and locked again"), !Door->IsOpen() && Door->IsLocked());

	// Without toggling, equipment stays on (Phase 11 behaviour).
	Generator->SetCanToggle(false);
	IOutbreakInteractable::Execute_Interact(Generator, Player);
	TestFalse(TEXT("One-way equipment cannot be focused once on"), IOutbreakInteractable::Execute_CanInteract(Generator, Player));
	return true;
}

// --- Destruction ---------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakDestructionTest, "Outbreak.Environment.Destruction.StagesAndExplosion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 * Damage stages, and an exploding object that hurts what is near it and sets off another one.
 * Uses engine radial damage, so it relies on physics overlaps working in the test world.
 */
bool FOutbreakDestructionTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakDestructibleObject* FuelTank = TestWorld.Spawn<AOutbreakDestructibleObject>(FVector::ZeroVector);
	AOutbreakCombatTestTarget* Bystander = TestWorld.Spawn<AOutbreakCombatTestTarget>(FVector(300.0f, 0.0f, 0.0f));
	AOutbreakDestructibleObject* Crate = TestWorld.Spawn<AOutbreakDestructibleObject>(FVector(0.0f, 400.0f, 0.0f));
	AOutbreakDestructibleObject* FarCrate = TestWorld.Spawn<AOutbreakDestructibleObject>(FVector(0.0f, 3000.0f, 0.0f));
	if (!FuelTank || !Bystander || !Crate || !FarCrate)
	{
		return false;
	}
	FuelTank->SetExplosion(500.0f, 600.0f);

	FuelTank->GetHealthComponent()->ApplyDamage(60.0f, nullptr);
	TestTrue(TEXT("Damaged stage"), FuelTank->IsDamaged() && !FuelTank->IsDestroyed());
	TestFalse(TEXT("Untouched crate is not damaged"), Crate->IsDamaged());

	FuelTank->GetHealthComponent()->ApplyDamage(50.0f, nullptr);
	TestTrue(TEXT("Destroyed"), FuelTank->IsDestroyed());
	TestFalse(TEXT("Destroyed object no longer blocks (cover gone)"), FuelTank->GetActorEnableCollision());
	TestTrue(TEXT("Blast hurts nearby soldiers and targets"),
		Bystander->GetHealthComponent()->GetHealth() < Bystander->GetHealthComponent()->GetMaxHealth());
	TestTrue(TEXT("Blast sets off nearby destructibles"), Crate->IsDestroyed());
	TestFalse(TEXT("Out of range: untouched"), FarCrate->IsDamaged());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
