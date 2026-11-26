#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/OutbreakPlayerCharacter.h"
#include "Tests/OutbreakTestWorld.h"
#include "UObject/Package.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakFireBehavior.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

namespace OutbreakWeaponTests
{
	UOutbreakAmmoTypeDefinition* MakeAmmo()
	{
		return NewObject<UOutbreakAmmoTypeDefinition>(GetTransientPackage());
	}

	/** A valid semi-automatic hitscan weapon with an instant reload. */
	UOutbreakWeaponDefinition* MakeWeapon(const TArray<UOutbreakAmmoTypeDefinition*>& AmmoTypes,
		int32 MagazineCapacity = 10, int32 FireModeCount = 1)
	{
		UOutbreakWeaponDefinition* Weapon = NewObject<UOutbreakWeaponDefinition>(GetTransientPackage());
		Weapon->MagazineCapacity = MagazineCapacity;
		Weapon->ReloadDuration = 0.0f;

		for (int32 Index = 0; Index < FireModeCount; ++Index)
		{
			FOutbreakFireModeDefinition& Mode = Weapon->FireModes.AddDefaulted_GetRef();
			Mode.FireBehavior = NewObject<UOutbreakHitscanFireBehavior>(Weapon);
			Mode.bAutomatic = false;
		}
		for (UOutbreakAmmoTypeDefinition* AmmoType : AmmoTypes)
		{
			Weapon->CompatibleAmmoTypes.Add(AmmoType);
		}
		return Weapon;
	}

	/** Spawns a player character and returns its weapon component (null on failure). */
	UOutbreakWeaponComponent* SpawnArmedCharacter(FOutbreakTestWorld& TestWorld)
	{
		AOutbreakPlayerCharacter* Character = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		return Character ? Character->GetWeaponComponent() : nullptr;
	}
}

using namespace OutbreakWeaponTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakWeaponDefinitionValidationTest, "Outbreak.Weapons.Definition.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakWeaponDefinitionValidationTest::RunTest(const FString& Parameters)
{
	TArray<FText> Errors;
	TestTrue(TEXT("Valid weapon passes"), MakeWeapon({ MakeAmmo() })->Validate(Errors));
	TestEqual(TEXT("No errors for valid weapon"), Errors.Num(), 0);

	UOutbreakWeaponDefinition* NoModes = MakeWeapon({ MakeAmmo() });
	NoModes->FireModes.Reset();
	Errors.Reset();
	TestFalse(TEXT("Weapon without fire modes fails"), NoModes->Validate(Errors));

	UOutbreakWeaponDefinition* NoAmmo = MakeWeapon({});
	Errors.Reset();
	TestFalse(TEXT("Weapon without ammunition types fails"), NoAmmo->Validate(Errors));

	UOutbreakWeaponDefinition* NoBehavior = MakeWeapon({ MakeAmmo() });
	NoBehavior->FireModes[0].FireBehavior = nullptr;
	Errors.Reset();
	TestFalse(TEXT("Fire mode without behaviour fails"), NoBehavior->Validate(Errors));

	UOutbreakWeaponDefinition* ExpensiveShot = MakeWeapon({ MakeAmmo() }, 5);
	ExpensiveShot->FireModes[0].AmmoPerShot = 6;
	Errors.Reset();
	TestFalse(TEXT("AmmoPerShot above capacity fails"), ExpensiveShot->Validate(Errors));

	UOutbreakAmmoTypeDefinition* BadAmmo = MakeAmmo();
	BadAmmo->DamageMultiplier = -1.0f;
	Errors.Reset();
	TestFalse(TEXT("Negative ammunition multiplier fails"), BadAmmo->Validate(Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakWeaponSpreadTest, "Outbreak.Weapons.Definition.SpreadAndFireRate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakWeaponSpreadTest::RunTest(const FString& Parameters)
{
	FOutbreakSpreadSettings Spread;
	Spread.HipSpread = 2.0f;
	Spread.AimSpread = 0.5f;
	Spread.MaxSpread = 6.0f;
	Spread.RecoveryPerSecond = 6.0f;

	TestEqual(TEXT("Hip spread without bloom"), Spread.GetSpread(false, 0.0f), 2.0f);
	TestEqual(TEXT("Aimed spread without bloom"), Spread.GetSpread(true, 0.0f), 0.5f);
	TestEqual(TEXT("Bloom adds to spread"), Spread.GetSpread(false, 1.0f), 3.0f);
	TestEqual(TEXT("Spread is capped"), Spread.GetSpread(false, 100.0f), 6.0f);
	TestEqual(TEXT("Bloom recovers over time"), Spread.RecoverBloom(2.0f, 0.1f), 1.4f, 0.001f);
	TestEqual(TEXT("Bloom never goes negative"), Spread.RecoverBloom(1.0f, 10.0f), 0.0f);

	FOutbreakFireModeDefinition Mode;
	Mode.RoundsPerMinute = 600.0f;
	TestEqual(TEXT("600 RPM is 0.1 s between shots"), Mode.GetFireInterval(), 0.1f, 0.0001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakWeaponEquipTest, "Outbreak.Weapons.Component.EquipAndSwitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakWeaponEquipTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	UOutbreakWeaponComponent* Weapons = SpawnArmedCharacter(TestWorld);
	if (!TestNotNull(TEXT("Weapon component"), Weapons))
	{
		return false;
	}

	TestEqual(TEXT("No weapon equipped initially"), Weapons->GetActiveWeaponIndex(), static_cast<int32>(INDEX_NONE));
	TestFalse(TEXT("Cannot fire without a weapon"), Weapons->CanFire());

	UOutbreakWeaponDefinition* Invalid = MakeWeapon({});
	TestEqual(TEXT("Invalid weapon rejected"), Weapons->AddWeapon(Invalid), static_cast<int32>(INDEX_NONE));

	UOutbreakWeaponDefinition* Rifle = MakeWeapon({ MakeAmmo() }, 30);
	UOutbreakWeaponDefinition* Pistol = MakeWeapon({ MakeAmmo() }, 12);

	TestEqual(TEXT("First weapon in slot 0"), Weapons->AddWeapon(Rifle), 0);
	TestEqual(TEXT("First weapon equipped"), Weapons->GetActiveWeaponIndex(), 0);
	TestEqual(TEXT("Starts with full magazine"), Weapons->GetAmmoInMagazine(), 30);
	TestEqual(TEXT("Duplicate returns existing slot"), Weapons->AddWeapon(Rifle), 0);

	TestEqual(TEXT("Second weapon in slot 1"), Weapons->AddWeapon(Pistol), 1);
	TestEqual(TEXT("Adding does not switch"), Weapons->GetActiveWeaponIndex(), 0);
	TestEqual(TEXT("Weapon count"), Weapons->GetWeaponCount(), 2);

	Weapons->EquipNextWeapon();
	TestEqual(TEXT("Next weapon"), Weapons->GetActiveWeaponIndex(), 1);
	TestTrue(TEXT("Active definition follows"), Weapons->GetActiveWeaponDefinition() == Pistol);
	TestEqual(TEXT("Pistol magazine"), Weapons->GetAmmoInMagazine(), 12);

	Weapons->EquipNextWeapon();
	TestEqual(TEXT("Next wraps around"), Weapons->GetActiveWeaponIndex(), 0);

	Weapons->EquipPreviousWeapon();
	TestEqual(TEXT("Previous wraps around"), Weapons->GetActiveWeaponIndex(), 1);

	Weapons->EquipWeapon(5);
	TestEqual(TEXT("Invalid slot ignored"), Weapons->GetActiveWeaponIndex(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakWeaponFireTest, "Outbreak.Weapons.Component.FireConsumesAmmoAndRespectsFireRate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakWeaponFireTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	UOutbreakWeaponComponent* Weapons = SpawnArmedCharacter(TestWorld);
	if (!TestNotNull(TEXT("Weapon component"), Weapons))
	{
		return false;
	}

	UOutbreakWeaponDefinition* Rifle = MakeWeapon({ MakeAmmo() }, 10);
	Rifle->FireModes[0].AmmoPerShot = 2;
	Weapons->AddWeapon(Rifle);

	Weapons->StartFire();
	Weapons->StopFire();
	TestEqual(TEXT("A shot consumes AmmoPerShot"), Weapons->GetAmmoInMagazine(), 8);

	// The test world clock does not advance, so a second shot is inside the fire interval.
	Weapons->StartFire();
	Weapons->StopFire();
	TestEqual(TEXT("Fire rate blocks an immediate second shot"), Weapons->GetAmmoInMagazine(), 8);

	Weapons->SetWeaponsEnabled(false);
	TestFalse(TEXT("Disabled weapons cannot fire"), Weapons->CanFire());
	Weapons->SetWeaponsEnabled(true);
	TestTrue(TEXT("Re-enabled weapons can fire"), Weapons->CanFire());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakWeaponEmptyMagazineTest, "Outbreak.Weapons.Component.EmptyMagazine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakWeaponEmptyMagazineTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	UOutbreakWeaponComponent* Weapons = SpawnArmedCharacter(TestWorld);
	if (!TestNotNull(TEXT("Weapon component"), Weapons))
	{
		return false;
	}

	UOutbreakAmmoTypeDefinition* Ammo = MakeAmmo();
	Weapons->AddWeapon(MakeWeapon({ Ammo }, 1));

	Weapons->StartFire();
	Weapons->StopFire();
	TestEqual(TEXT("Magazine empty"), Weapons->GetAmmoInMagazine(), 0);
	TestFalse(TEXT("Cannot fire with empty magazine"), Weapons->CanFire());
	TestFalse(TEXT("Cannot reload without reserve"), Weapons->CanReload());

	// Firing on empty triggers the automatic reload once reserve exists (instant reload here).
	Weapons->AddAmmo(Ammo, 3);
	Weapons->StartFire();
	Weapons->StopFire();
	TestEqual(TEXT("Auto-reload filled the magazine"), Weapons->GetAmmoInMagazine(), 1);
	TestEqual(TEXT("Auto-reload took from reserve"), Weapons->GetReserveAmmo(Ammo), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakWeaponReloadTest, "Outbreak.Weapons.Component.Reload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakWeaponReloadTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Empty magazine, plenty of reserve"), UOutbreakWeaponComponent::CalculateReloadAmount(30, 0, 100), 30);
	TestEqual(TEXT("Partial magazine"), UOutbreakWeaponComponent::CalculateReloadAmount(30, 25, 100), 5);
	TestEqual(TEXT("Reserve smaller than gap"), UOutbreakWeaponComponent::CalculateReloadAmount(30, 10, 5), 5);
	TestEqual(TEXT("Full magazine"), UOutbreakWeaponComponent::CalculateReloadAmount(30, 30, 10), 0);
	TestEqual(TEXT("No reserve"), UOutbreakWeaponComponent::CalculateReloadAmount(30, 0, 0), 0);
	TestEqual(TEXT("Overfull magazine"), UOutbreakWeaponComponent::CalculateReloadAmount(30, 35, 10), 0);
	TestEqual(TEXT("Negative reserve"), UOutbreakWeaponComponent::CalculateReloadAmount(30, 0, -5), 0);

	FOutbreakTestWorld TestWorld;
	UOutbreakWeaponComponent* Weapons = SpawnArmedCharacter(TestWorld);
	if (!TestNotNull(TEXT("Weapon component"), Weapons))
	{
		return false;
	}

	UOutbreakAmmoTypeDefinition* Ammo = MakeAmmo();
	Weapons->AddWeapon(MakeWeapon({ Ammo }, 10));
	Weapons->AddAmmo(Ammo, 5);

	TestFalse(TEXT("Cannot reload a full magazine"), Weapons->CanReload());

	Weapons->StartFire();
	Weapons->StopFire();
	TestEqual(TEXT("One round fired"), Weapons->GetAmmoInMagazine(), 9);

	Weapons->StartReload();
	TestFalse(TEXT("Instant reload finished"), Weapons->IsReloading());
	TestEqual(TEXT("Magazine refilled"), Weapons->GetAmmoInMagazine(), 10);
	TestEqual(TEXT("Reserve reduced by one"), Weapons->GetReserveAmmo(Ammo), 4);

	Weapons->AddAmmo(nullptr, 10);
	Weapons->AddAmmo(Ammo, -3);
	TestEqual(TEXT("Invalid AddAmmo ignored"), Weapons->GetReserveAmmo(Ammo), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakWeaponFireModeTest, "Outbreak.Weapons.Component.FireModeSwitching",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakWeaponFireModeTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	UOutbreakWeaponComponent* Weapons = SpawnArmedCharacter(TestWorld);
	if (!TestNotNull(TEXT("Weapon component"), Weapons))
	{
		return false;
	}

	UOutbreakWeaponDefinition* Rifle = MakeWeapon({ MakeAmmo() }, 10, 2);
	Rifle->FireModes[1].AmmoPerShot = 3;
	Weapons->AddWeapon(Rifle);

	FOutbreakWeaponSlot Slot;
	Weapons->GetActiveWeaponSlot(Slot);
	TestEqual(TEXT("Starts in primary mode"), Slot.FireModeIndex, 0);

	Weapons->CycleFireMode();
	Weapons->GetActiveWeaponSlot(Slot);
	TestEqual(TEXT("Alternative mode selected"), Slot.FireModeIndex, 1);
	TestTrue(TEXT("Active fire mode follows"), Weapons->GetActiveFireMode() == &Rifle->FireModes[1]);

	Weapons->StartFire();
	Weapons->StopFire();
	TestEqual(TEXT("Alternative mode uses its own AmmoPerShot"), Weapons->GetAmmoInMagazine(), 7);

	Weapons->CycleFireMode();
	Weapons->GetActiveWeaponSlot(Slot);
	TestEqual(TEXT("Wraps back to primary"), Slot.FireModeIndex, 0);

	// A single-mode weapon ignores cycling.
	Weapons->AddWeapon(MakeWeapon({ MakeAmmo() }));
	Weapons->EquipWeapon(1);
	Weapons->CycleFireMode();
	Weapons->GetActiveWeaponSlot(Slot);
	TestEqual(TEXT("Single mode unchanged"), Slot.FireModeIndex, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakWeaponAmmoTypeTest, "Outbreak.Weapons.Component.AmmoTypeSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakWeaponAmmoTypeTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	UOutbreakWeaponComponent* Weapons = SpawnArmedCharacter(TestWorld);
	if (!TestNotNull(TEXT("Weapon component"), Weapons))
	{
		return false;
	}

	UOutbreakAmmoTypeDefinition* Standard = MakeAmmo();
	UOutbreakAmmoTypeDefinition* Piercing = MakeAmmo();
	UOutbreakAmmoTypeDefinition* Incompatible = MakeAmmo();
	Weapons->AddWeapon(MakeWeapon({ Standard, Piercing }, 10));
	Weapons->AddAmmo(Piercing, 25);
	Weapons->AddAmmo(Incompatible, 50);

	Weapons->CycleAmmoType();

	FOutbreakWeaponSlot Slot;
	Weapons->GetActiveWeaponSlot(Slot);
	TestTrue(TEXT("Piercing selected"), Slot.SelectedAmmoType == Piercing);
	TestTrue(TEXT("Piercing loaded by the reload"), Slot.LoadedAmmoType == Piercing);
	TestEqual(TEXT("Magazine full of piercing"), Slot.AmmoInMagazine, 10);
	TestEqual(TEXT("Standard rounds returned to reserve"), Weapons->GetReserveAmmo(Standard), 10);
	TestEqual(TEXT("Piercing reserve reduced"), Weapons->GetReserveAmmo(Piercing), 15);
	TestEqual(TEXT("Incompatible reserve untouched"), Weapons->GetReserveAmmo(Incompatible), 50);

	Weapons->CycleAmmoType();
	Weapons->GetActiveWeaponSlot(Slot);
	TestTrue(TEXT("Cycles back to standard"), Slot.LoadedAmmoType == Standard);
	TestEqual(TEXT("Piercing rounds returned"), Weapons->GetReserveAmmo(Piercing), 25);
	TestEqual(TEXT("Standard reserve used"), Weapons->GetReserveAmmo(Standard), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
