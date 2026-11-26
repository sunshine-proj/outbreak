#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/OutbreakEnemyDefinition.h"
#include "Characters/OutbreakCharacterAnimInstance.h"
#include "Characters/OutbreakEnemyCharacter.h"
#include "Combat/OutbreakCombatEvents.h"
#include "Combat/OutbreakCombatTestTarget.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "Environment/OutbreakSwitchable.h"
#include "Game/OutbreakPlayerState.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/OutbreakAlarmSpeaker.h"
#include "Missions/OutbreakAlarmSubsystem.h"
#include "Missions/OutbreakEncounter.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "Missions/OutbreakMissionMarker.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Missions/OutbreakReachLocationObjective.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Squad/OutbreakSquadCommandTypes.h"
#include "Squad/OutbreakSquadComms.h"
#include "Squad/OutbreakSquadCommsComponent.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakSquadRoleDefinition.h"
#include "Squad/OutbreakTeammateCharacter.h"
#include "Tests/OutbreakTestWorld.h"
#include "UI/OutbreakHUDText.h"
#include "UObject/Package.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakFireBehavior.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

namespace OutbreakVerticalSliceTests
{
	UOutbreakWeaponDefinition* MakeWeapon(UOutbreakAmmoTypeDefinition* Ammo, EOutbreakWeaponPose Pose = EOutbreakWeaponPose::Rifle)
	{
		UOutbreakWeaponDefinition* Weapon = NewObject<UOutbreakWeaponDefinition>(GetTransientPackage());
		Weapon->MagazineCapacity = 10;
		Weapon->ReloadDuration = 0.0f;
		Weapon->WeaponPose = Pose;
		FOutbreakFireModeDefinition& Mode = Weapon->FireModes.AddDefaulted_GetRef();
		Mode.FireBehavior = NewObject<UOutbreakHitscanFireBehavior>(Weapon);
		Mode.bAutomatic = false;
		Weapon->CompatibleAmmoTypes.Add(Ammo);
		return Weapon;
	}

	UOutbreakMissionDefinition* MakeMission(bool bAlarm, float AlarmDelay)
	{
		UOutbreakMissionDefinition* Mission = NewObject<UOutbreakMissionDefinition>(GetTransientPackage());
		Mission->bAlarmEnabled = bAlarm;
		Mission->AlarmDelay = AlarmDelay;
		UOutbreakReachLocationObjective* Reach = NewObject<UOutbreakReachLocationObjective>(Mission);
		Reach->Description = FText::FromString(TEXT("Reach the gate"));
		Reach->ObjectiveId = TEXT("Gate");
		Reach->MarkerId = TEXT("Gate");
		Mission->Objectives.Add(Reach);
		return Mission;
	}

	UOutbreakVoiceDefinition* MakeVoice()
	{
		UOutbreakVoiceDefinition* Voice = NewObject<UOutbreakVoiceDefinition>(GetTransientPackage());
		Voice->Lines.Add(EOutbreakCallout::Acknowledge).Subtitles.Add(FText::FromString(TEXT("Copy.")));
		Voice->Lines.Add(EOutbreakCallout::Reloading).Subtitles.Add(FText::FromString(TEXT("Reloading!")));
		Voice->Lines.Add(EOutbreakCallout::ManDown).Subtitles.Add(FText::FromString(TEXT("{Name} is down!")));
		return Voice;
	}

	/** Player plus Hawk and Viper, who speak with a test voice. */
	struct FCommsSquad
	{
		AOutbreakPlayerState* PlayerState = nullptr;
		AOutbreakPlayerCharacter* Player = nullptr;
		AOutbreakTeammateCharacter* Hawk = nullptr;
		AOutbreakTeammateCharacter* Viper = nullptr;

		bool Create(FOutbreakTestWorld& TestWorld)
		{
			PlayerState = TestWorld.Spawn<AOutbreakPlayerState>();
			Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
			Hawk = TestWorld.Spawn<AOutbreakTeammateCharacter>(FVector(300.0f, 0.0f, 0.0f));
			Viper = TestWorld.Spawn<AOutbreakTeammateCharacter>(FVector(-300.0f, 0.0f, 0.0f));
			if (!PlayerState || !Player || !Hawk || !Viper)
			{
				return false;
			}
			UOutbreakSquadRoleDefinition* Role = NewObject<UOutbreakSquadRoleDefinition>(GetTransientPackage());
			Role->Voice = MakeVoice();
			UOutbreakSquadComponent* Squad = PlayerState->GetSquadComponent();
			Squad->RegisterPlayerMember(Player->GetSquadMemberComponent());
			for (AOutbreakTeammateCharacter* Teammate : { Hawk, Viper })
			{
				Squad->RegisterMember(Teammate->GetSquadMemberComponent());
				Teammate->GetSquadMemberComponent()->SetRole(Role);
			}
			Hawk->GetSquadMemberComponent()->SetDisplayName(FText::FromString(TEXT("Hawk")));
			Viper->GetSquadMemberComponent()->SetDisplayName(FText::FromString(TEXT("Viper")));
			PlayerState->GetSquadCommsComponent()->BindEvents();
			return true;
		}

		FString LastSubtitle() const { return PlayerState->GetSquadCommsComponent()->GetLastSubtitle().ToString(); }
	};
}

using namespace OutbreakVerticalSliceTests;

// --- Presentation --------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakWeaponActionDetectTest, "Outbreak.Presentation.WeaponActions.Detect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakWeaponActionDetectTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakWeaponActions;
	UOutbreakAmmoTypeDefinition* Ammo = NewObject<UOutbreakAmmoTypeDefinition>(GetTransientPackage());
	const UOutbreakWeaponDefinition* Rifle = MakeWeapon(Ammo);
	const UOutbreakWeaponDefinition* Pistol = MakeWeapon(Ammo, EOutbreakWeaponPose::Pistol);

	TestEqual(TEXT("First weapon given is not an equip"), DetectStateActions(false, nullptr, false, Rifle).Num(), 0);
	TestTrue(TEXT("Switching weapons is an equip"),
		DetectStateActions(false, Rifle, false, Pistol) == TArray<EOutbreakWeaponAction>({ EOutbreakWeaponAction::Equipped }));
	TestTrue(TEXT("Reload starting"),
		DetectStateActions(false, Rifle, true, Rifle) == TArray<EOutbreakWeaponAction>({ EOutbreakWeaponAction::ReloadStarted }));
	TestEqual(TEXT("Reload continuing or ending is nothing new"), DetectStateActions(true, Rifle, false, Rifle).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakAnimStateTest, "Outbreak.Presentation.Animation.GatherState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** The Animation Blueprint state follows the weapon, health and enemy state. */
bool FOutbreakAnimStateTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakEnemyCharacter* Enemy = TestWorld.Spawn<AOutbreakEnemyCharacter>(FVector(1000.0f, 0.0f, 0.0f));
	if (!Player || !Enemy)
	{
		return false;
	}

	FOutbreakCharacterAnimState State;
	UOutbreakCharacterAnimInstance::GatherState(Player, State);
	TestTrue(TEXT("Unarmed"), !State.bHasWeapon && State.WeaponPose == EOutbreakWeaponPose::Unarmed);

	UOutbreakAmmoTypeDefinition* Ammo = NewObject<UOutbreakAmmoTypeDefinition>(GetTransientPackage());
	Player->GetWeaponComponent()->AddWeapon(MakeWeapon(Ammo, EOutbreakWeaponPose::Pistol));
	UOutbreakCharacterAnimInstance::GatherState(Player, State);
	TestTrue(TEXT("Weapon pose from the definition"), State.bHasWeapon && State.WeaponPose == EOutbreakWeaponPose::Pistol);
	TestFalse(TEXT("Standing still"), State.Speed > 0.0f);

	Player->GetHealthComponent()->ConfigureIncapacitation(true, 30.0f, 0);
	Player->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	UOutbreakCharacterAnimInstance::GatherState(Player, State);
	TestTrue(TEXT("Downed pose"), State.bIsDowned && !State.bIsDead);

	Enemy->SetEnemyState(EOutbreakEnemyState::Combat);
	UOutbreakCharacterAnimInstance::GatherState(Enemy, State);
	TestTrue(TEXT("Enemy state and combat stance"), State.EnemyState == EOutbreakEnemyState::Combat && State.bCombatReady);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakAwarenessTextTest, "Outbreak.Presentation.HUD.AwarenessText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakAwarenessTextTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakHUDText;
	TestTrue(TEXT("Unnoticed: nothing"), BuildAwarenessText(0.0f, false).IsEmpty());
	TestEqual(TEXT("Being noticed"), BuildAwarenessText(0.35f, false).ToString(), FString(TEXT("NOTICED [####------]")));
	TestEqual(TEXT("Almost detected never shows a full meter"), BuildAwarenessText(0.99f, false).ToString(), FString(TEXT("NOTICED [#########-]")));
	TestEqual(TEXT("Detected"), BuildAwarenessText(1.0f, false).ToString(), FString(TEXT("DETECTED")));
	TestEqual(TEXT("Alarm alone"), BuildAwarenessText(0.0f, true).ToString(), FString(TEXT("ALARM RAISED")));
	TestEqual(TEXT("Detected and alarm"), BuildAwarenessText(1.0f, true).ToString(), FString(TEXT("DETECTED\nALARM RAISED")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakDamageDealtEventTest, "Outbreak.Presentation.HUD.DamageDealtEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Hit markers come from this event: every applied hit, and whether it took the target down. */
bool FOutbreakDamageDealtEventTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakCombatTestTarget* Target = TestWorld.Spawn<AOutbreakCombatTestTarget>();
	if (!Target)
	{
		return false;
	}

	int32 Hits = 0;
	bool bLastTakenDown = false;
	float LastDamage = 0.0f;
	const FDelegateHandle Handle = OutbreakCombatEvents::OnDamageDealt().AddLambda(
		[&](AActor* Victim, AController* Instigator, float Damage, bool bTakenDown)
		{
			if (Victim == Target)
			{
				++Hits;
				LastDamage = Damage;
				bLastTakenDown = bTakenDown;
			}
		});

	UGameplayStatics::ApplyDamage(Target, 10.0f, nullptr, nullptr, UDamageType::StaticClass());
	TestEqual(TEXT("Hit reported"), Hits, 1);
	TestEqual(TEXT("Damage reported"), LastDamage, 10.0f);
	TestFalse(TEXT("Not a kill"), bLastTakenDown);

	UGameplayStatics::ApplyDamage(Target, 10000.0f, nullptr, nullptr, UDamageType::StaticClass());
	TestTrue(TEXT("Killing hit reported as such"), Hits == 2 && bLastTakenDown);

	UGameplayStatics::ApplyDamage(Target, 10.0f, nullptr, nullptr, UDamageType::StaticClass());
	TestEqual(TEXT("Hitting the dead does nothing"), Hits, 2);

	OutbreakCombatEvents::OnDamageDealt().Remove(Handle);
	return true;
}

// --- Alarm ---------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakAlarmTest, "Outbreak.Missions.Alarm.DelayAndSilencing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Detection raises the alarm after the delay unless the spotter is silenced; panels reset it; missions can turn it off. */
bool FOutbreakAlarmTest::RunTest(const FString& Parameters)
{
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakEnemyCharacter* Enemy = TestWorld.Spawn<AOutbreakEnemyCharacter>();
		UOutbreakAlarmSubsystem* Alarm = TestWorld.Get()->GetSubsystem<UOutbreakAlarmSubsystem>();
		TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>()->InitializeMission(MakeMission(true, 0.0f));

		Enemy->SetEnemyState(EOutbreakEnemyState::Combat);
		TestTrue(TEXT("No delay: alarm at once"), Alarm->IsAlarmRaised());

		AOutbreakAlarmSpeaker* Panel = TestWorld.Spawn<AOutbreakAlarmSpeaker>();
		IOutbreakSwitchable::Execute_SetSwitchedOn(Panel, true, nullptr);
		TestFalse(TEXT("Alarm panel silences it"), Alarm->IsAlarmRaised());
	}
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakEnemyCharacter* Spotter = TestWorld.Spawn<AOutbreakEnemyCharacter>();
		AOutbreakEnemyCharacter* Second = TestWorld.Spawn<AOutbreakEnemyCharacter>(FVector(500.0f, 0.0f, 0.0f));
		UOutbreakAlarmSubsystem* Alarm = TestWorld.Get()->GetSubsystem<UOutbreakAlarmSubsystem>();
		TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>()->InitializeMission(MakeMission(true, 4.0f));

		Spotter->SetEnemyState(EOutbreakEnemyState::Combat);
		TestTrue(TEXT("Countdown running"), Alarm->IsAlarmPending() && !Alarm->IsAlarmRaised());
		Spotter->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
		Alarm->ResolvePendingAlarm();
		TestFalse(TEXT("Spotter silenced in time: no alarm"), Alarm->IsAlarmRaised());

		Second->SetEnemyState(EOutbreakEnemyState::Combat);
		TestTrue(TEXT("Next detection starts a new countdown"), Alarm->IsAlarmPending());
		Alarm->ResolvePendingAlarm();
		TestTrue(TEXT("Spotter still fighting: alarm"), Alarm->IsAlarmRaised());
	}
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakEnemyCharacter* Enemy = TestWorld.Spawn<AOutbreakEnemyCharacter>();
		UOutbreakAlarmSubsystem* Alarm = TestWorld.Get()->GetSubsystem<UOutbreakAlarmSubsystem>();
		Enemy->SetEnemyState(EOutbreakEnemyState::Combat);
		TestFalse(TEXT("Free play: no alarm"), Alarm->IsAlarmPending() || Alarm->IsAlarmRaised());

		TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>()->InitializeMission(MakeMission(false, 0.0f));
		Alarm->ReportCombatStarted(Enemy);
		TestFalse(TEXT("Mission without alarm: no alarm"), Alarm->IsAlarmPending() || Alarm->IsAlarmRaised());
	}
	return true;
}

// --- Encounters ----------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEncounterTest, "Outbreak.Missions.Encounter.SpawnAndTriggers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Encounters spawn their groups once at their spawn points, tagged, on manual, objective and alarm triggers. */
bool FOutbreakEncounterTest::RunTest(const FString& Parameters)
{
	UOutbreakEnemyDefinition* Infantry = NewObject<UOutbreakEnemyDefinition>(GetTransientPackage());
	FOutbreakEncounterGroup Group;
	Group.Definition = Infantry;
	Group.Count = 3;

	{
		FOutbreakTestWorld TestWorld;
		ATargetPoint* PointA = TestWorld.Spawn<ATargetPoint>(FVector(2000.0f, 0.0f, 0.0f));
		ATargetPoint* PointB = TestWorld.Spawn<ATargetPoint>(FVector(-2000.0f, 0.0f, 0.0f));
		AOutbreakEncounter* Encounter = TestWorld.Spawn<AOutbreakEncounter>();
		Encounter->Configure(EOutbreakEncounterTrigger::Manual, NAME_None, { Group }, { PointA, PointB }, TEXT("Counterattack"),
			0.0f, EOutbreakEncounterOrder::Guard);

		TestFalse(TEXT("Not cleared before spawning"), Encounter->IsCleared());
		Encounter->Trigger();
		const TArray<AOutbreakEnemyCharacter*> Enemies = Encounter->GetSpawnedEnemies();
		if (!TestEqual(TEXT("Group spawned"), Enemies.Num(), 3))
		{
			return false;
		}
		TestTrue(TEXT("Tagged for objectives"), Enemies[0]->ActorHasTag(TEXT("Counterattack")));
		TestTrue(TEXT("Archetype applied"), Enemies[0]->GetEnemyDefinition() == Infantry);
		TestTrue(TEXT("At the spawn points"), FVector::Dist(Enemies[0]->GetActorLocation(), PointA->GetActorLocation()) < 400.0f
			&& FVector::Dist(Enemies[1]->GetActorLocation(), PointB->GetActorLocation()) < 400.0f);
		TestTrue(TEXT("Third enemy beside the first point, not on it"),
			FVector::Dist2D(Enemies[2]->GetActorLocation(), PointA->GetActorLocation()) > 50.0f);

		Encounter->Trigger();
		TestEqual(TEXT("Fires once"), Encounter->GetSpawnedEnemies().Num(), 3);

		for (AOutbreakEnemyCharacter* Enemy : Enemies)
		{
			Enemy->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
		}
		TestTrue(TEXT("Cleared"), Encounter->IsCleared());
	}
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		TestWorld.Spawn<AOutbreakMissionMarker>()->SetMarkerId(TEXT("Gate"));
		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(MakeMission(true, 0.0f));
		Mission->RegisterPlayer(Player);

		AOutbreakEncounter* OnActivated = TestWorld.Spawn<AOutbreakEncounter>(FVector(3000.0f, 0.0f, 0.0f));
		OnActivated->Configure(EOutbreakEncounterTrigger::ObjectiveActivated, TEXT("Gate"), { Group }, {}, NAME_None, 0.0f, EOutbreakEncounterOrder::Guard);
		OnActivated->BindTrigger();
		AOutbreakEncounter* OnCompleted = TestWorld.Spawn<AOutbreakEncounter>(FVector(-3000.0f, 0.0f, 0.0f));
		OnCompleted->Configure(EOutbreakEncounterTrigger::ObjectiveCompleted, TEXT("Gate"), { Group }, {}, NAME_None, 0.0f, EOutbreakEncounterOrder::Guard);
		OnCompleted->BindTrigger();
		AOutbreakEncounter* OnAlarm = TestWorld.Spawn<AOutbreakEncounter>(FVector(0.0f, 3000.0f, 0.0f));
		OnAlarm->Configure(EOutbreakEncounterTrigger::Alarm, NAME_None, { Group }, {}, NAME_None, 0.0f, EOutbreakEncounterOrder::Guard);
		OnAlarm->BindTrigger();

		Mission->StartMission();
		TestTrue(TEXT("Objective activated: reinforcements"), OnActivated->HasTriggered());
		TestFalse(TEXT("Not yet completed"), OnCompleted->HasTriggered());

		Mission->UpdateMission();
		TestTrue(TEXT("Objective completed: counterattack"), OnCompleted->HasTriggered());
		TestFalse(TEXT("No alarm yet"), OnAlarm->HasTriggered());

		TestWorld.Get()->GetSubsystem<UOutbreakAlarmSubsystem>()->RaiseAlarm(FVector::ZeroVector, nullptr);
		TestTrue(TEXT("Alarm: guards arrive"), OnAlarm->HasTriggered() && OnAlarm->GetSpawnedEnemies().Num() == 3);
	}
	return true;
}

// --- Squad communication -------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadCommsRulesTest, "Outbreak.Squad.Comms.Pacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakSquadCommsRulesTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakSquadComms;
	FOutbreakSquadCommsSettings Settings;
	Settings.MinInterval = 2.0f;
	Settings.SameCalloutCooldown = 8.0f;

	TestTrue(TEXT("First callout"), IsCalloutAllowed(EOutbreakCallout::Contact, 10.0, -1.0e9, -1.0e9, Settings));
	TestFalse(TEXT("Too soon after another callout"), IsCalloutAllowed(EOutbreakCallout::Contact, 11.0, 10.0, -1.0e9, Settings));
	TestTrue(TEXT("Urgent callouts cut in"), IsCalloutAllowed(EOutbreakCallout::ManDown, 11.0, 10.0, -1.0e9, Settings));
	TestFalse(TEXT("Same callout repeats only after its cooldown"), IsCalloutAllowed(EOutbreakCallout::ManDown, 15.0, 10.0, 10.0, Settings));
	TestTrue(TEXT("After the cooldown"), IsCalloutAllowed(EOutbreakCallout::Contact, 19.0, 10.0, 10.0, Settings));

	TestEqual(TEXT("Subtitle with the squadmate's name"),
		FormatSubtitle(FText::FromString(TEXT("Hawk")), FText::FromString(TEXT("{Name} is down!")), FText::FromString(TEXT("Viper"))).ToString(),
		FString(TEXT("Hawk: Viper is down!")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadCommsCalloutTest, "Outbreak.Squad.Comms.Callouts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Orders are acknowledged, chatter is paced, a squadmate going down is called out by someone else, reloads are called. */
bool FOutbreakSquadCommsCalloutTest::RunTest(const FString& Parameters)
{
	{
		FOutbreakTestWorld TestWorld;
		FCommsSquad Squad;
		if (!TestTrue(TEXT("Squad"), Squad.Create(TestWorld)))
		{
			return false;
		}
		UOutbreakSquadCommsComponent* Comms = Squad.PlayerState->GetSquadCommsComponent();

		Squad.PlayerState->GetSquadComponent()->AuthIssueCommand(FOutbreakSquadCommand::Make(EOutbreakSquadCommandType::Hold),
			FOutbreakCommandRecipients::ForSquad());
		TestEqual(TEXT("Order acknowledged"), Squad.LastSubtitle(), FString(TEXT("Hawk: Copy.")));

		TestFalse(TEXT("Routine chatter paced out"), Comms->RequestCallout(EOutbreakCallout::Reloading, Squad.Viper->GetSquadMemberComponent()));

		Squad.Viper->GetHealthComponent()->ConfigureIncapacitation(true, 30.0f, 0);
		Squad.Viper->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
		TestEqual(TEXT("Man down, called by another teammate"), Squad.LastSubtitle(), FString(TEXT("Hawk: Viper is down!")));

		TestFalse(TEXT("A voice without the line says nothing"), Comms->RequestCallout(EOutbreakCallout::AlarmRaised));
	}
	{
		FOutbreakTestWorld TestWorld;
		FCommsSquad Squad;
		if (!TestTrue(TEXT("Squad"), Squad.Create(TestWorld)))
		{
			return false;
		}
		UOutbreakAmmoTypeDefinition* Ammo = NewObject<UOutbreakAmmoTypeDefinition>(GetTransientPackage());
		UOutbreakWeaponComponent* Weapons = Squad.Viper->GetWeaponComponent();
		Weapons->AddWeapon(MakeWeapon(Ammo));
		Weapons->AddAmmo(Ammo, 20);
		Weapons->StartFire();
		Weapons->StopFire();
		Weapons->StartReload();
		TestEqual(TEXT("Reload called out by the reloading teammate"), Squad.LastSubtitle(), FString(TEXT("Viper: Reloading!")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
