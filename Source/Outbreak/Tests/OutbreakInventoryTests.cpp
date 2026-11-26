#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/OutbreakMedicalAssist.h"
#include "AI/OutbreakSquadAIController.h"
#include "AI/OutbreakTeammateBehaviorTypes.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Combat/OutbreakMedicalComponent.h"
#include "Combat/OutbreakMedicalSettings.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/OutbreakPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/OutbreakInteractable.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Inventory/OutbreakInventoryText.h"
#include "Inventory/OutbreakItemAction.h"
#include "Inventory/OutbreakItemDefinition.h"
#include "Inventory/OutbreakLoadout.h"
#include "Inventory/OutbreakPickup.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "Missions/OutbreakMissionMarker.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Missions/OutbreakReachLocationObjective.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakSquadRoleDefinition.h"
#include "Squad/OutbreakTeammateCharacter.h"
#include "Tests/OutbreakTestWorld.h"
#include "UObject/Package.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakFireBehavior.h"
#include "Weapons/OutbreakProjectile.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

namespace OutbreakInventoryTests
{
	UOutbreakItemDefinition* MakeMedkit(float HealAmount = 40.0f)
	{
		UOutbreakItemDefinition* Medkit = NewObject<UOutbreakItemDefinition>(GetTransientPackage());
		Medkit->DisplayName = FText::FromString(TEXT("Medkit"));
		Medkit->Category = EOutbreakItemCategory::Medical;
		Medkit->Weight = 0.5f;
		Medkit->MaxStack = 3;
		UOutbreakHealItemAction* Heal = NewObject<UOutbreakHealItemAction>(Medkit);
		Heal->HealAmount = HealAmount;
		Medkit->UseAction = Heal;
		return Medkit;
	}

	UOutbreakItemDefinition* MakeGrenade()
	{
		UOutbreakItemDefinition* Grenade = NewObject<UOutbreakItemDefinition>(GetTransientPackage());
		Grenade->DisplayName = FText::FromString(TEXT("Grenade"));
		Grenade->Category = EOutbreakItemCategory::Utility;
		Grenade->Weight = 0.4f;
		Grenade->MaxStack = 4;
		Grenade->UseAction = NewObject<UOutbreakThrowItemAction>(Grenade);
		return Grenade;
	}

	UOutbreakAmmoTypeDefinition* MakeAmmo(float WeightPerRound = 0.01f)
	{
		UOutbreakAmmoTypeDefinition* Ammo = NewObject<UOutbreakAmmoTypeDefinition>(GetTransientPackage());
		Ammo->DisplayName = FText::FromString(TEXT("Rifle rounds"));
		Ammo->WeightPerRound = WeightPerRound;
		return Ammo;
	}

	UOutbreakWeaponDefinition* MakeWeapon(UOutbreakAmmoTypeDefinition* Ammo, EOutbreakEquipmentSlot Slot, float Weight, int32 Magazine = 30)
	{
		UOutbreakWeaponDefinition* Weapon = NewObject<UOutbreakWeaponDefinition>(GetTransientPackage());
		Weapon->DisplayName = FText::FromString(Slot == EOutbreakEquipmentSlot::Primary ? TEXT("Rifle") : TEXT("Pistol"));
		Weapon->EquipmentSlot = Slot;
		Weapon->Weight = Weight;
		Weapon->MagazineCapacity = Magazine;
		Weapon->ReloadDuration = 0.0f;
		FOutbreakFireModeDefinition& Mode = Weapon->FireModes.AddDefaulted_GetRef();
		Mode.FireBehavior = NewObject<UOutbreakHitscanFireBehavior>(Weapon);
		Weapon->CompatibleAmmoTypes.Add(Ammo);
		return Weapon;
	}

	/** A player and a teammate in one squad. */
	struct FSquadFixture
	{
		AOutbreakPlayerState* PlayerState = nullptr;
		AOutbreakPlayerCharacter* Player = nullptr;
		AOutbreakTeammateCharacter* Teammate = nullptr;
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
			Squad = PlayerState->GetSquadComponent();
			Squad->RegisterPlayerMember(Player->GetSquadMemberComponent());
			Squad->RegisterMember(Teammate->GetSquadMemberComponent());
			return true;
		}
	};
}

using namespace OutbreakInventoryTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakInventoryCapacityTest, "Outbreak.Inventory.Capacity.AddableCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Weight and stack limits decide how much fits. */
bool FOutbreakInventoryCapacityTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakInventory;
	TestEqual(TEXT("Plenty of room"), ComputeAddableCount(0.5f, 0.0f, 10.0f, 0, 0, 4), 4);
	TestEqual(TEXT("Weight limits"), ComputeAddableCount(1.0f, 7.5f, 10.0f, 0, 0, 5), 2);
	TestEqual(TEXT("Stack limits"), ComputeAddableCount(0.1f, 0.0f, 10.0f, 2, 3, 5), 1);
	TestEqual(TEXT("Full stack"), ComputeAddableCount(0.1f, 0.0f, 10.0f, 3, 3, 5), 0);
	TestEqual(TEXT("Exactly full by weight (rounding)"), ComputeAddableCount(0.1f, 0.0f, 1.0f, 0, 0, 10), 10);
	TestEqual(TEXT("Over capacity already"), ComputeAddableCount(1.0f, 12.0f, 10.0f, 0, 0, 1), 0);
	TestEqual(TEXT("Weightless items only stack-limited"), ComputeAddableCount(0.0f, 50.0f, 10.0f, 0, 2, 5), 2);
	TestEqual(TEXT("Nothing requested"), ComputeAddableCount(1.0f, 0.0f, 10.0f, 0, 0, 0), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakInventoryContentsTest, "Outbreak.Inventory.Component.CollectAndRemove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Items stack and weigh; ammunition goes to the weapon reserve within the weight limit; removal. */
bool FOutbreakInventoryContentsTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	UOutbreakInventoryComponent* Inventory = Player ? Player->GetInventoryComponent() : nullptr;
	if (!TestNotNull(TEXT("Inventory on soldiers"), Inventory))
	{
		return false;
	}
	Inventory->SetMaxCarryWeight(5.0f);

	UOutbreakItemDefinition* Medkit = MakeMedkit();
	TestEqual(TEXT("Stack limit: 3 of 5 medkits"), Inventory->AddItem(Medkit, 5), 3);
	TestEqual(TEXT("Medkits carried"), Inventory->GetItemCount(Medkit), 3);
	TestEqual(TEXT("Weight of medkits"), static_cast<double>(Inventory->GetCarriedWeight()), 1.5, 0.001);

	UOutbreakAmmoTypeDefinition* Ammo = MakeAmmo(0.01f);
	TestEqual(TEXT("Ammunition limited by the remaining 3.5 kg"), Inventory->AddItem(Ammo, 1000), 350);
	TestEqual(TEXT("Ammunition is in the weapon reserve"), Player->GetWeaponComponent()->GetReserveAmmo(Ammo), 350);
	TestEqual(TEXT("Full"), Inventory->GetAddableCount(Medkit, 1), 0);

	TestEqual(TEXT("Remove one medkit"), Inventory->RemoveItem(Medkit, 1), 1);
	TestEqual(TEXT("Two left"), Inventory->GetItemCount(Medkit), 2);
	TestEqual(TEXT("Remove ammunition"), Inventory->RemoveItem(Ammo, 100), 100);
	TestEqual(TEXT("Reserve reduced"), Player->GetWeaponComponent()->GetReserveAmmo(Ammo), 250);
	TestEqual(TEXT("Room again"), Inventory->GetAddableCount(Medkit, 1), 1);

	TestEqual(TEXT("Removing more than carried removes what there is"), Inventory->RemoveItem(Medkit, 10), 2);
	TestEqual(TEXT("Entry gone"), Inventory->GetItems().Num(), 0);
	TestEqual(TEXT("Not an item"), Inventory->AddItem(Player, 1), 0);

	const FText Compact = OutbreakInventoryText::BuildInventoryText(Inventory, false);
	TestTrue(TEXT("Summary shows the weight"), Compact.ToString().Contains(TEXT("/ 5.0 kg")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakInventorySlotsTest, "Outbreak.Inventory.Equipment.SlotsAndSwap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** One primary and one secondary weapon; swapping replaces the slot's weapon; weapons and magazines weigh. */
bool FOutbreakInventorySlotsTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	UOutbreakInventoryComponent* Inventory = Player ? Player->GetInventoryComponent() : nullptr;
	if (!TestNotNull(TEXT("Inventory"), Inventory))
	{
		return false;
	}
	Inventory->SetMaxCarryWeight(20.0f);

	UOutbreakAmmoTypeDefinition* Ammo = MakeAmmo(0.01f);
	UOutbreakWeaponDefinition* Rifle = MakeWeapon(Ammo, EOutbreakEquipmentSlot::Primary, 4.0f, 30);
	UOutbreakWeaponDefinition* Shotgun = MakeWeapon(Ammo, EOutbreakEquipmentSlot::Primary, 3.0f, 8);
	UOutbreakWeaponDefinition* Pistol = MakeWeapon(Ammo, EOutbreakEquipmentSlot::Secondary, 1.0f, 12);

	TestEqual(TEXT("Rifle into the primary slot"), Inventory->AddItem(Rifle, 1), 1);
	TestEqual(TEXT("Pistol into the secondary slot"), Inventory->AddItem(Pistol, 1), 1);
	TestTrue(TEXT("Primary slot"), Inventory->GetWeaponInSlot(EOutbreakEquipmentSlot::Primary) == Rifle);
	TestTrue(TEXT("Secondary slot"), Inventory->GetWeaponInSlot(EOutbreakEquipmentSlot::Secondary) == Pistol);
	TestEqual(TEXT("Second primary does not fit"), Inventory->AddItem(Shotgun, 1), 0);

	// 4 + 1 kg of weapons, plus 30 + 12 loaded rounds at 0.01 kg.
	TestEqual(TEXT("Weapons and magazines weigh"), static_cast<double>(Inventory->GetCarriedWeight()), 5.42, 0.001);

	UOutbreakWeaponDefinition* Replaced = nullptr;
	TestTrue(TEXT("Swap the rifle for the shotgun"), Inventory->SwapWeapon(Shotgun, Replaced));
	TestTrue(TEXT("Rifle replaced"), Replaced == Rifle);
	TestTrue(TEXT("Shotgun in the primary slot"), Inventory->GetWeaponInSlot(EOutbreakEquipmentSlot::Primary) == Shotgun);
	TestEqual(TEXT("Rifle's loaded rounds kept as reserve"), Player->GetWeaponComponent()->GetReserveAmmo(Ammo), 30);

	Inventory->SetMaxCarryWeight(4.5f);
	UOutbreakWeaponDefinition* HeavyRifle = MakeWeapon(Ammo, EOutbreakEquipmentSlot::Primary, 6.0f, 30);
	TestFalse(TEXT("A swap that is too heavy fails"), Inventory->SwapWeapon(HeavyRifle, Replaced));
	TestTrue(TEXT("Shotgun kept"), Inventory->GetWeaponInSlot(EOutbreakEquipmentSlot::Primary) == Shotgun);

	TestEqual(TEXT("Drop the pistol"), Inventory->RemoveItem(Pistol, 1), 1);
	TestNull(TEXT("Secondary slot empty"), Inventory->GetWeaponInSlot(EOutbreakEquipmentSlot::Secondary));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakPickupTest, "Outbreak.Inventory.Pickup.Collection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Picking up takes what fits and leaves the rest; a weapon pickup swaps; drops create pickups. */
bool FOutbreakPickupTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakPickup* MedkitPickup = TestWorld.Spawn<AOutbreakPickup>(FVector(100.0f, 0.0f, 0.0f));
	AOutbreakPickup* WeaponPickup = TestWorld.Spawn<AOutbreakPickup>(FVector(100.0f, 100.0f, 0.0f));
	if (!TestNotNull(TEXT("Player"), Player) || !MedkitPickup || !WeaponPickup)
	{
		return false;
	}
	UOutbreakInventoryComponent* Inventory = Player->GetInventoryComponent();

	UOutbreakItemDefinition* Medkit = MakeMedkit();
	MedkitPickup->SetContents(Medkit, 5);
	TestTrue(TEXT("Pickup prompt"), IOutbreakInteractable::Execute_GetInteractionPrompt(MedkitPickup, Player).ToString().Contains(TEXT("Medkit")));
	TestTrue(TEXT("Can pick up"), IOutbreakInteractable::Execute_CanInteract(MedkitPickup, Player));
	IOutbreakInteractable::Execute_Interact(MedkitPickup, Player);
	TestEqual(TEXT("Took a full stack"), Inventory->GetItemCount(Medkit), 3);
	TestEqual(TEXT("The rest stays"), MedkitPickup->GetCount(), 2);
	TestFalse(TEXT("Nothing more fits"), IOutbreakInteractable::Execute_CanInteract(MedkitPickup, Player));

	UOutbreakAmmoTypeDefinition* Ammo = MakeAmmo();
	UOutbreakWeaponDefinition* Rifle = MakeWeapon(Ammo, EOutbreakEquipmentSlot::Primary, 4.0f);
	UOutbreakWeaponDefinition* Carbine = MakeWeapon(Ammo, EOutbreakEquipmentSlot::Primary, 3.0f);
	Inventory->AddItem(Rifle, 1);
	WeaponPickup->SetContents(Carbine, 1);
	TestTrue(TEXT("Swap prompt"), IOutbreakInteractable::Execute_GetInteractionPrompt(WeaponPickup, Player).ToString().StartsWith(TEXT("Swap")));
	IOutbreakInteractable::Execute_Interact(WeaponPickup, Player);
	TestTrue(TEXT("Carbine taken"), Inventory->GetWeaponInSlot(EOutbreakEquipmentSlot::Primary) == Carbine);
	TestTrue(TEXT("Rifle left on the ground"), WeaponPickup->GetItem() == Rifle);

	AOutbreakPickup* Dropped = Inventory->DropItem(Medkit, 1);
	TestNotNull(TEXT("Dropping spawns a pickup"), Dropped);
	if (Dropped)
	{
		TestEqual(TEXT("Dropped count"), Dropped->GetCount(), 1);
	}
	TestEqual(TEXT("Two medkits left"), Inventory->GetItemCount(Medkit), 2);

	UOutbreakItemDefinition* Keycard = NewObject<UOutbreakItemDefinition>(GetTransientPackage());
	Keycard->Category = EOutbreakItemCategory::Mission;
	Keycard->bCanDrop = false;
	Inventory->AddItem(Keycard, 1);
	TestNull(TEXT("Mission items cannot be dropped"), Inventory->DropItem(Keycard, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakLoadoutTest, "Outbreak.Inventory.Loadout.ApplyAndValidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Loadouts weigh, validate slots and capacity, and replace a soldier's equipment. */
bool FOutbreakLoadoutTest::RunTest(const FString& Parameters)
{
	UOutbreakAmmoTypeDefinition* Ammo = MakeAmmo(0.01f);
	UOutbreakItemDefinition* Medkit = MakeMedkit();

	FOutbreakLoadout Loadout;
	Loadout.PrimaryWeapon = MakeWeapon(Ammo, EOutbreakEquipmentSlot::Primary, 4.0f, 30);
	Loadout.SecondaryWeapon = MakeWeapon(Ammo, EOutbreakEquipmentSlot::Secondary, 1.0f, 10);
	Loadout.Ammo.Add({ Ammo, 200 });
	FOutbreakItemAmount& Medkits = Loadout.Items.AddDefaulted_GetRef();
	Medkits.Item = Medkit;
	Medkits.Count = 2;
	Loadout.CarryCapacity = 12.0f;

	// 5 kg weapons + 40 loaded + 200 reserve rounds (2.4 kg) + 1 kg medkits.
	TestEqual(TEXT("Loadout weight"), static_cast<double>(OutbreakLoadout::ComputeWeight(Loadout)), 8.4, 0.001);
	TArray<FText> Errors;
	TestTrue(TEXT("Valid loadout"), OutbreakLoadout::Validate(Loadout, 0.0f, Errors));

	FOutbreakLoadout Overloaded = Loadout;
	Overloaded.CarryCapacity = 6.0f;
	Errors.Reset();
	TestFalse(TEXT("Over capacity rejected"), OutbreakLoadout::Validate(Overloaded, 0.0f, Errors));

	FOutbreakLoadout WrongSlot = Loadout;
	WrongSlot.PrimaryWeapon = Loadout.SecondaryWeapon;
	Errors.Reset();
	TestFalse(TEXT("Pistol in the primary slot rejected"), OutbreakLoadout::Validate(WrongSlot, 0.0f, Errors));

	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	if (!TestNotNull(TEXT("Player"), Player))
	{
		return false;
	}
	UOutbreakInventoryComponent* Inventory = Player->GetInventoryComponent();
	UOutbreakWeaponDefinition* OldWeapon = MakeWeapon(Ammo, EOutbreakEquipmentSlot::Primary, 2.0f);
	Inventory->AddItem(OldWeapon, 1);
	Inventory->AddItem(MakeGrenade(), 2);

	Inventory->ApplyLoadout(Loadout);
	TestTrue(TEXT("Primary from the loadout"), Inventory->GetWeaponInSlot(EOutbreakEquipmentSlot::Primary) == Loadout.PrimaryWeapon);
	TestTrue(TEXT("Primary in hand"), Player->GetWeaponComponent()->GetActiveWeaponDefinition() == Loadout.PrimaryWeapon);
	TestEqual(TEXT("Old weapon gone"), Inventory->GetItemCount(OldWeapon), 0);
	TestEqual(TEXT("Old items gone, loadout items in"), Inventory->GetItems().Num(), 1);
	TestEqual(TEXT("Medkits from the loadout"), Inventory->GetItemCount(Medkit), 2);
	TestEqual(TEXT("Reserve from the loadout"), Player->GetWeaponComponent()->GetReserveAmmo(Ammo), 200);
	TestEqual(TEXT("Capacity from the loadout"), static_cast<double>(Inventory->GetMaxCarryWeight()), 12.0, 0.001);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakHealthMedicalTest, "Outbreak.Medical.Health.HealReviveBleedOut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Healing, revive, bleed-out and the revive limit. Squad soldiers are downable by default. */
bool FOutbreakHealthMedicalTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakTeammateCharacter* Soldier = TestWorld.Spawn<AOutbreakTeammateCharacter>();
	if (!TestNotNull(TEXT("Soldier"), Soldier))
	{
		return false;
	}
	UOutbreakHealthComponent* Health = Soldier->GetHealthComponent();

	if (GetDefault<UOutbreakMedicalSettings>()->bSquadCanBeIncapacitated)
	{
		Health->ApplyDamage(10000.0f, nullptr);
		TestTrue(TEXT("Squad soldiers go down by default"), Health->IsIncapacitated());
		Health->Revive(1.0f);
	}

	Health->ConfigureIncapacitation(true, 30.0f, 1);
	Health->ResetHealth();

	Health->ApplyDamage(60.0f, nullptr);
	TestEqual(TEXT("Heal restores health"), Health->Heal(25.0f), 25.0f);
	TestEqual(TEXT("Heal capped at maximum"), Health->Heal(100.0f), 35.0f);

	Health->ApplyDamage(10000.0f, nullptr);
	TestTrue(TEXT("Down"), Health->IsIncapacitated());
	TestEqual(TEXT("No healing while down"), Health->Heal(50.0f), 0.0f);
	TestEqual(TEXT("Bleed-out countdown"), static_cast<double>(Health->GetBleedOutTimeRemaining()), 30.0, 0.5);
	TestTrue(TEXT("Weapons disabled while down"), !Soldier->GetWeaponComponent()->AreWeaponsEnabled());

	TestTrue(TEXT("Revive"), Health->Revive(0.3f));
	TestTrue(TEXT("Alive again"), Health->IsAlive());
	TestEqual(TEXT("Revived at 30%"), Health->GetHealth(), 30.0f);
	TestEqual(TEXT("Revive counted"), Health->GetReviveCount(), 1);
	TestEqual(TEXT("No revives left"), Health->GetRevivesRemaining(), 0);
	TestTrue(TEXT("Weapons back"), Soldier->GetWeaponComponent()->AreWeaponsEnabled());
	TestTrue(TEXT("Can move again"), Soldier->GetCharacterMovement()->MovementMode == MOVE_Walking);
	TestFalse(TEXT("Revive only when down"), Health->Revive(0.5f));

	Health->ApplyDamage(10000.0f, nullptr);
	TestTrue(TEXT("Out of revives: killed outright"), Health->GetHealthState() == EOutbreakHealthState::Dead);

	Health->ConfigureIncapacitation(true, 30.0f, 0);
	Health->ResetHealth();
	Health->ApplyDamage(10000.0f, nullptr);
	Health->FinishBleedOut();
	TestTrue(TEXT("Bleeding out kills"), Health->GetHealthState() == EOutbreakHealthState::Dead);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakMedicalItemsTest, "Outbreak.Medical.Items.HealAndUtility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Medkits heal yourself or a nearby squadmate (more as a medic); grenades are thrown; uses are consumed. */
bool FOutbreakMedicalItemsTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	FSquadFixture Fixture;
	if (!TestTrue(TEXT("Squad created"), Fixture.Create(TestWorld, FVector(150.0f, 0.0f, 0.0f))))
	{
		return false;
	}
	UOutbreakInventoryComponent* Inventory = Fixture.Player->GetInventoryComponent();
	UOutbreakItemDefinition* Medkit = MakeMedkit(40.0f);
	Inventory->AddItem(Medkit, 3);

	TestFalse(TEXT("No use at full health"), Inventory->UseMedicalOn(Fixture.Player));
	Fixture.Player->GetHealthComponent()->ApplyDamage(50.0f, nullptr);
	TestTrue(TEXT("Heal yourself"), Inventory->UseMedicalOn(Fixture.Player));
	TestEqual(TEXT("Healed 40"), Fixture.Player->GetHealthComponent()->GetHealth(), 90.0f);
	TestEqual(TEXT("One medkit used"), Inventory->GetItemCount(Medkit), 2);

	Fixture.Teammate->GetHealthComponent()->ApplyDamage(70.0f, nullptr);
	TestTrue(TEXT("Heal prompt on a hurt squadmate"),
		IOutbreakInteractable::Execute_GetInteractionPrompt(Fixture.Teammate, Fixture.Player).ToString().StartsWith(TEXT("Heal")));
	IOutbreakInteractable::Execute_Interact(Fixture.Teammate, Fixture.Player);
	TestEqual(TEXT("Squadmate healed"), Fixture.Teammate->GetHealthComponent()->GetHealth(), 70.0f);

	Fixture.Teammate->SetActorLocation(FVector(5000.0f, 0.0f, 0.0f));
	TestFalse(TEXT("Out of reach"), Inventory->CanUseItem(Medkit, Fixture.Teammate));

	// A medic heals more.
	UOutbreakSquadRoleDefinition* Medic = NewObject<UOutbreakSquadRoleDefinition>(GetTransientPackage());
	Medic->MedicalEffectMultiplier = 1.5f;
	Fixture.Player->GetSquadMemberComponent()->SetRole(Medic);
	Fixture.Player->GetHealthComponent()->ApplyDamage(80.0f, nullptr);
	Inventory->UseMedicalOn(Fixture.Player);
	TestEqual(TEXT("Medic heals 60"), Fixture.Player->GetHealthComponent()->GetHealth(), 70.0f);
	TestEqual(TEXT("Medkits are a limited resource"), Inventory->GetItemCount(Medkit), 0);
	TestFalse(TEXT("None left"), Inventory->UseMedicalOn(Fixture.Player));

	// Utility: a thrown grenade.
	UOutbreakItemDefinition* Grenade = MakeGrenade();
	Inventory->AddItem(Grenade, 2);
	TestTrue(TEXT("Grenade selected"), Inventory->GetSelectedUtility() == Grenade);
	int32 ProjectilesBefore = 0;
	for (TActorIterator<AOutbreakProjectile> It(TestWorld.Get()); It; ++It)
	{
		++ProjectilesBefore;
	}
	TestTrue(TEXT("Throw"), Inventory->UseSelectedUtility());
	int32 ProjectilesAfter = 0;
	for (TActorIterator<AOutbreakProjectile> It(TestWorld.Get()); It; ++It)
	{
		++ProjectilesAfter;
	}
	TestEqual(TEXT("A projectile was thrown"), ProjectilesAfter, ProjectilesBefore + 1);
	TestEqual(TEXT("One grenade left"), Inventory->GetItemCount(Grenade), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakReviveTest, "Outbreak.Medical.Revive.Interaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** The player revives a downed teammate through interaction; leaving the patient cancels it. */
bool FOutbreakReviveTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	FSquadFixture Fixture;
	if (!TestTrue(TEXT("Squad created"), Fixture.Create(TestWorld, FVector(120.0f, 0.0f, 0.0f))))
	{
		return false;
	}
	UOutbreakHealthComponent* PatientHealth = Fixture.Teammate->GetHealthComponent();
	PatientHealth->ConfigureIncapacitation(true, 30.0f, 0);
	PatientHealth->ApplyDamage(10000.0f, nullptr);
	if (!TestTrue(TEXT("Teammate down"), PatientHealth->IsIncapacitated()))
	{
		return false;
	}

	UOutbreakMedicalComponent* Medical = Fixture.Player->GetMedicalComponent();
	TestTrue(TEXT("Revive prompt"),
		IOutbreakInteractable::Execute_GetInteractionPrompt(Fixture.Teammate, Fixture.Player).ToString().StartsWith(TEXT("Revive")));
	TestTrue(TEXT("Can revive"), IOutbreakInteractable::Execute_CanInteract(Fixture.Teammate, Fixture.Player));

	// Out of range: the revive is cancelled.
	IOutbreakInteractable::Execute_Interact(Fixture.Teammate, Fixture.Player);
	TestTrue(TEXT("Reviving"), Medical->IsReviving());
	TestTrue(TEXT("Status text"), OutbreakInventoryText::BuildMedicalStatusText(Fixture.Player).ToString().StartsWith(TEXT("Reviving")));
	TestTrue(TEXT("Patient sees it"), OutbreakInventoryText::BuildMedicalStatusText(Fixture.Teammate).ToString().StartsWith(TEXT("Being revived")));
	Fixture.Player->SetActorLocation(FVector(3000.0f, 0.0f, 0.0f));
	Medical->CompleteRevive();
	TestFalse(TEXT("Moving away cancels"), Medical->IsReviving());
	TestTrue(TEXT("Still down"), PatientHealth->IsIncapacitated());

	// In range: the revive completes.
	Fixture.Player->SetActorLocation(FVector::ZeroVector);
	IOutbreakInteractable::Execute_Interact(Fixture.Teammate, Fixture.Player);
	Medical->CompleteRevive();
	TestTrue(TEXT("Revived"), PatientHealth->IsAlive());
	TestEqual(TEXT("Revive health"), static_cast<double>(PatientHealth->GetHealthFraction()),
		static_cast<double>(GetDefault<UOutbreakMedicalSettings>()->ReviveHealthFraction), 0.01);
	TestFalse(TEXT("Done"), Medical->IsReviving());
	TestTrue(TEXT("Member active again"), Fixture.Teammate->GetSquadMemberComponent()->GetStatus() == EOutbreakSquadMemberStatus::Active);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakAssistRulesTest, "Outbreak.Medical.TeammateAI.AssistRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Who helps whom, and how medical assistance ranks against orders. */
bool FOutbreakAssistRulesTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakMedicalAssist;
	const FVector Self(0.0f, 0.0f, 0.0f);

	FOutbreakAssistPatient Hurt;
	Hurt.Location = FVector(500.0f, 0.0f, 0.0f);
	Hurt.HealthFraction = 0.3f;
	FOutbreakAssistPatient Downed;
	Downed.Location = FVector(1500.0f, 0.0f, 0.0f);
	Downed.bDowned = true;
	Downed.HealthFraction = 0.0f;

	TestEqual(TEXT("Downed beats hurt"), ChoosePatient(Self, TArray<FOutbreakAssistPatient>{ Hurt, Downed }, 3000.0f, true, 0.5f), 1);
	TestEqual(TEXT("Hurt only with a medkit"), ChoosePatient(Self, TArray<FOutbreakAssistPatient>{ Hurt }, 3000.0f, false, 0.5f),
		static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("Heal the hurt"), ChoosePatient(Self, TArray<FOutbreakAssistPatient>{ Hurt }, 3000.0f, true, 0.5f), 0);

	FOutbreakAssistPatient Taken = Downed;
	Taken.bBeingRevivedByOther = true;
	TestEqual(TEXT("Already being revived"), ChoosePatient(Self, TArray<FOutbreakAssistPatient>{ Taken }, 3000.0f, true, 0.5f),
		static_cast<int32>(INDEX_NONE));
	FOutbreakAssistPatient Closer = Downed;
	Closer.NearestOtherHelperDistance = 200.0f;
	TestEqual(TEXT("Someone nearer goes"), ChoosePatient(Self, TArray<FOutbreakAssistPatient>{ Closer }, 3000.0f, true, 0.5f),
		static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("Too far"), ChoosePatient(Self, TArray<FOutbreakAssistPatient>{ Downed }, 1000.0f, true, 0.5f),
		static_cast<int32>(INDEX_NONE));

	const TArray<EOutbreakBehaviorLayer> Order = OutbreakTeammateBehavior::GetDefaultPriorityOrder();
	TestTrue(TEXT("Default order valid with the medical layer"), OutbreakTeammateBehavior::IsValidPriorityOrder(Order));

	FOutbreakBehaviorContext Context;
	Context.bHasPlayer = true;
	Context.ActiveCommand = EOutbreakSquadCommandType::Hold;
	Context.bHasDownedSquadmate = true;
	TestTrue(TEXT("Reviving beats an order"),
		OutbreakTeammateBehavior::SelectBehavior(Context, Order).Behavior == EOutbreakTeammateBehavior::ReviveMember);
	Context.bHasDownedSquadmate = false;
	Context.bHasHurtSquadmate = true;
	TestTrue(TEXT("Healing yields to an order"),
		OutbreakTeammateBehavior::SelectBehavior(Context, Order).Behavior == EOutbreakTeammateBehavior::HoldPosition);
	Context.ActiveCommand = EOutbreakSquadCommandType::None;
	TestTrue(TEXT("Healing without an order"),
		OutbreakTeammateBehavior::SelectBehavior(Context, Order).Behavior == EOutbreakTeammateBehavior::HealMember);
	Context.bLowHealth = true;
	Context.bHasThreat = true;
	TestTrue(TEXT("Own survival first"),
		OutbreakTeammateBehavior::SelectBehavior(Context, Order).Behavior == EOutbreakTeammateBehavior::SeekSafety);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakTeammateReviveTest, "Outbreak.Medical.TeammateAI.RevivesDownedPlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** A teammate goes to the downed player, starts the revive on arrival, and heals itself when hurt. */
bool FOutbreakTeammateReviveTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	FSquadFixture Fixture;
	if (!TestTrue(TEXT("Squad created"), Fixture.Create(TestWorld, FVector(1500.0f, 0.0f, 0.0f))))
	{
		return false;
	}
	if (!Fixture.Teammate->GetController())
	{
		Fixture.Teammate->SpawnDefaultController();
	}
	AOutbreakSquadAIController* Controller = Cast<AOutbreakSquadAIController>(Fixture.Teammate->GetController());
	if (!TestNotNull(TEXT("Teammate controller"), Controller))
	{
		return false;
	}

	UOutbreakHealthComponent* PlayerHealth = Fixture.Player->GetHealthComponent();
	PlayerHealth->ConfigureIncapacitation(true, 30.0f, 0);
	PlayerHealth->ApplyDamage(10000.0f, nullptr);

	Controller->UpdateBehavior();
	TestTrue(TEXT("Goes to revive the player"), Controller->GetCurrentBehavior() == EOutbreakTeammateBehavior::ReviveMember);
	TestTrue(TEXT("Patient is the player"), Controller->GetAssistPatient() == Fixture.Player);
	TestTrue(TEXT("Heads for the player"), Controller->GetMoveLocation().Equals(Fixture.Player->GetActorLocation(), 1.0));

	Fixture.Teammate->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	Controller->UpdateBehavior();
	UOutbreakMedicalComponent* Medical = Fixture.Teammate->GetMedicalComponent();
	TestTrue(TEXT("Starts the revive on arrival"), Medical->IsReviving() && Medical->GetRevivePatient() == Fixture.Player);
	Medical->CompleteRevive();
	TestTrue(TEXT("Player revived"), PlayerHealth->IsAlive());

	Controller->UpdateBehavior();
	TestTrue(TEXT("Back to normal"), Controller->GetCurrentBehavior() != EOutbreakTeammateBehavior::ReviveMember);

	// Self-heal when hurt and not under fire.
	UOutbreakItemDefinition* Medkit = MakeMedkit(30.0f);
	Fixture.Teammate->GetInventoryComponent()->AddItem(Medkit, 1);
	Fixture.Teammate->GetHealthComponent()->ApplyDamage(80.0f, nullptr);
	TestEqual(TEXT("Teammate hurt"), Fixture.Teammate->GetHealthComponent()->GetHealth(), 20.0f);
	Controller->UpdateBehavior();
	TestEqual(TEXT("Teammate used its medkit"), Fixture.Teammate->GetInventoryComponent()->GetItemCount(Medkit), 0);
	TestEqual(TEXT("Teammate healed"), Fixture.Teammate->GetHealthComponent()->GetHealth(), 50.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadDownMissionTest, "Outbreak.Mission.Flow.SquadDownFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** A downed player is not a failure while a squadmate can revive them; the whole squad down is. */
bool FOutbreakSquadDownMissionTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	FSquadFixture Fixture;
	if (!TestTrue(TEXT("Squad created"), Fixture.Create(TestWorld, FVector(500.0f, 0.0f, 0.0f))))
	{
		return false;
	}

	UOutbreakMissionDefinition* Definition = NewObject<UOutbreakMissionDefinition>(GetTransientPackage());
	Definition->RestartDelay = 0.0f;
	UOutbreakReachLocationObjective* Reach = NewObject<UOutbreakReachLocationObjective>(Definition);
	Reach->MarkerId = TEXT("FarAway");
	Reach->Radius = 200.0f;
	Definition->Objectives.Add(Reach);
	TestWorld.Spawn<AOutbreakMissionMarker>(FVector(50000.0f, 0.0f, 0.0f))->SetMarkerId(TEXT("FarAway"));

	UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
	Mission->InitializeMission(Definition);
	Mission->RegisterPlayer(Fixture.Player);
	Mission->StartMission();

	Fixture.Player->GetHealthComponent()->ConfigureIncapacitation(true, 30.0f, 0);
	Fixture.Teammate->GetHealthComponent()->ConfigureIncapacitation(true, 30.0f, 0);

	Fixture.Player->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	TestTrue(TEXT("Player down, teammate standing: mission goes on"), Mission->GetMissionState() == EOutbreakMissionState::InProgress);

	Fixture.Teammate->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	Mission->UpdateMission();
	TestTrue(TEXT("Whole squad down: failed"), Mission->GetMissionState() == EOutbreakMissionState::Failed);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
