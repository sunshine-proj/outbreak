#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/OutbreakEnemyCharacter.h"
#include "Combat/OutbreakCombatTestTarget.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Engine/World.h"
#include "Game/OutbreakPlayerState.h"
#include "Interaction/OutbreakInteractable.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Inventory/OutbreakItemDefinition.h"
#include "Inventory/OutbreakLoadout.h"
#include "Missions/OutbreakActivateEquipmentObjective.h"
#include "Missions/OutbreakDestroyObjectObjective.h"
#include "Missions/OutbreakDestructibleObject.h"
#include "Missions/OutbreakEliminateTargetObjective.h"
#include "Missions/OutbreakExtractionObjective.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "Missions/OutbreakMissionEquipment.h"
#include "Missions/OutbreakMissionMarker.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Missions/OutbreakMissionText.h"
#include "Missions/OutbreakObjectiveGraph.h"
#include "Missions/OutbreakProtectLocationObjective.h"
#include "Missions/OutbreakReachLocationObjective.h"
#include "Missions/OutbreakRetrieveItemObjective.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadConfiguration.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakSquadSelection.h"
#include "Squad/OutbreakTeammateCharacter.h"
#include "Tests/OutbreakTestWorld.h"
#include "UObject/Package.h"

namespace OutbreakMissionFrameworkTests
{
	UOutbreakMissionDefinition* MakeDefinition()
	{
		UOutbreakMissionDefinition* Definition = NewObject<UOutbreakMissionDefinition>(GetTransientPackage());
		Definition->DisplayName = FText::FromString(TEXT("Framework Mission"));
		Definition->RestartDelay = 0.0f;
		return Definition;
	}

	template <typename ObjectiveType>
	ObjectiveType* AddObjective(UOutbreakMissionDefinition* Definition, const TCHAR* Description, FName Id = NAME_None,
		EOutbreakObjectiveActivation Activation = EOutbreakObjectiveActivation::AfterPrevious, bool bOptional = false)
	{
		ObjectiveType* Objective = NewObject<ObjectiveType>(Definition);
		Objective->Description = FText::FromString(Description);
		Objective->ObjectiveId = Id;
		Objective->Activation = Activation;
		Objective->bOptional = bOptional;
		Definition->Objectives.Add(Objective);
		return Objective;
	}

	UOutbreakReachLocationObjective* AddReach(UOutbreakMissionDefinition* Definition, const TCHAR* Description, FName MarkerId,
		FName Id = NAME_None, EOutbreakObjectiveActivation Activation = EOutbreakObjectiveActivation::AfterPrevious, bool bOptional = false)
	{
		UOutbreakReachLocationObjective* Reach = AddObjective<UOutbreakReachLocationObjective>(Definition, Description, Id, Activation, bOptional);
		Reach->MarkerId = MarkerId;
		Reach->Radius = 200.0f;
		return Reach;
	}

	AOutbreakMissionMarker* SpawnMarker(FOutbreakTestWorld& TestWorld, FName MarkerId, const FVector& Location)
	{
		AOutbreakMissionMarker* Marker = TestWorld.Spawn<AOutbreakMissionMarker>(Location);
		Marker->SetMarkerId(MarkerId);
		return Marker;
	}

	FOutbreakObjectiveNode Node(FName Id, EOutbreakObjectiveActivation Activation, TArray<FName> Prerequisites = {}, bool bOptional = false)
	{
		FOutbreakObjectiveNode Result;
		Result.Id = Id;
		Result.Activation = Activation;
		Result.Prerequisites = MoveTemp(Prerequisites);
		Result.bOptional = bOptional;
		return Result;
	}

	UOutbreakItemDefinition* MakeMissionItem(const TCHAR* Name)
	{
		UOutbreakItemDefinition* Item = NewObject<UOutbreakItemDefinition>(GetTransientPackage());
		Item->DisplayName = FText::FromString(Name);
		Item->Category = EOutbreakItemCategory::Mission;
		Item->Weight = 0.5f;
		Item->MaxStack = 3;
		Item->bCanDrop = false;
		return Item;
	}

	/** A player and one teammate in one squad. */
	struct FSquad
	{
		AOutbreakPlayerState* PlayerState = nullptr;
		AOutbreakPlayerCharacter* Player = nullptr;
		AOutbreakTeammateCharacter* Teammate = nullptr;

		bool Create(FOutbreakTestWorld& TestWorld, const FVector& TeammateLocation)
		{
			PlayerState = TestWorld.Spawn<AOutbreakPlayerState>();
			Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
			Teammate = TestWorld.Spawn<AOutbreakTeammateCharacter>(TeammateLocation);
			if (!PlayerState || !Player || !Teammate)
			{
				return false;
			}
			PlayerState->GetSquadComponent()->RegisterPlayerMember(Player->GetSquadMemberComponent());
			PlayerState->GetSquadComponent()->RegisterMember(Teammate->GetSquadMemberComponent());
			Teammate->GetSquadMemberComponent()->SetDisplayName(FText::FromString(TEXT("Hawk")));
			return true;
		}
	};
}

using namespace OutbreakMissionFrameworkTests;

// --- Dependencies --------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakObjectiveGraphTest, "Outbreak.Mission.Dependencies.GraphRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** AfterPrevious skips optional objectives; prerequisites resolve by ID; invalid graphs are rejected. */
bool FOutbreakObjectiveGraphTest::RunTest(const FString& Parameters)
{
	using EA = EOutbreakObjectiveActivation;
	using namespace OutbreakObjectiveGraph;

	TArray<TArray<int32>> Prerequisites;
	TArray<FText> Errors;

	// 0 required, 1 optional (after 0), 2 required after previous = 0 (skips the optional one), 3 after {1}, 4 at start.
	const TArray<FOutbreakObjectiveNode> Nodes = {
		Node(TEXT("A"), EA::AfterPrevious),
		Node(TEXT("Bonus"), EA::AfterPrevious, {}, true),
		Node(TEXT("B"), EA::AfterPrevious),
		Node(TEXT("BonusFollowUp"), EA::AfterPrerequisites, { TEXT("Bonus") }, true),
		Node(NAME_None, EA::AtMissionStart),
	};
	if (!TestTrue(TEXT("Valid graph resolves"), Resolve(Nodes, Prerequisites, Errors)))
	{
		return false;
	}
	TestEqual(TEXT("First objective has no prerequisite"), Prerequisites[0].Num(), 0);
	TestTrue(TEXT("Optional after previous follows A"), Prerequisites[1] == TArray<int32>{ 0 });
	TestTrue(TEXT("AfterPrevious skips the optional objective"), Prerequisites[2] == TArray<int32>{ 0 });
	TestTrue(TEXT("Prerequisite by ID"), Prerequisites[3] == TArray<int32>{ 1 });
	TestEqual(TEXT("At mission start"), Prerequisites[4].Num(), 0);

	using ES = EOutbreakObjectiveState;
	TArray<ES> States = { ES::Completed, ES::Failed, ES::Inactive, ES::Inactive, ES::Active };
	TestTrue(TEXT("B ready once A completed"), ArePrerequisitesMet(2, Prerequisites, States));
	TestFalse(TEXT("Follow-up not ready"), ArePrerequisitesMet(3, Prerequisites, States));
	TestTrue(TEXT("Follow-up blocked by failed Bonus"), IsBlocked(3, Prerequisites, States));
	TestFalse(TEXT("B not blocked"), IsBlocked(2, Prerequisites, States));

	auto Rejects = [this](const TCHAR* What, const TArray<FOutbreakObjectiveNode>& Invalid)
	{
		TArray<TArray<int32>> Unused;
		TArray<FText> InvalidErrors;
		TestFalse(What, Resolve(Invalid, Unused, InvalidErrors));
		TestTrue(FString::Printf(TEXT("%s: has a reason"), What), InvalidErrors.Num() > 0);
	};
	Rejects(TEXT("Duplicate ID"), { Node(TEXT("A"), EA::AtMissionStart), Node(TEXT("A"), EA::AtMissionStart) });
	Rejects(TEXT("Unknown prerequisite"), { Node(TEXT("A"), EA::AfterPrerequisites, { TEXT("Missing") }) });
	Rejects(TEXT("Self prerequisite"), { Node(TEXT("A"), EA::AfterPrerequisites, { TEXT("A") }) });
	Rejects(TEXT("No prerequisites listed"), { Node(TEXT("A"), EA::AfterPrerequisites) });
	Rejects(TEXT("Cycle"), { Node(TEXT("A"), EA::AfterPrerequisites, { TEXT("B") }), Node(TEXT("B"), EA::AfterPrerequisites, { TEXT("A") }) });
	Rejects(TEXT("Required on optional"), { Node(TEXT("Opt"), EA::AtMissionStart, {}, true), Node(TEXT("A"), EA::AfterPrerequisites, { TEXT("Opt") }) });
	Rejects(TEXT("Only optional objectives"), { Node(TEXT("Opt"), EA::AtMissionStart, {}, true) });

	// The definition runs the same rules.
	UOutbreakMissionDefinition* Cyclic = MakeDefinition();
	AddReach(Cyclic, TEXT("A"), TEXT("M"), TEXT("A"), EA::AfterPrerequisites)->Prerequisites = { TEXT("B") };
	AddReach(Cyclic, TEXT("B"), TEXT("M"), TEXT("B"), EA::AfterPrerequisites)->Prerequisites = { TEXT("A") };
	Errors.Reset();
	TestFalse(TEXT("Definition with a cycle is invalid"), Cyclic->Validate(Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakObjectiveDependencyFlowTest, "Outbreak.Mission.Dependencies.ConcurrentAndOptional",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 * Several objectives run at once; a final objective waits for two prerequisites; optional failures
 * (and the objectives depending on them) do not fail the mission.
 */
bool FOutbreakObjectiveDependencyFlowTest::RunTest(const FString& Parameters)
{
	using EA = EOutbreakObjectiveActivation;
	using ES = EOutbreakObjectiveState;

	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakMissionMarker* Exit = SpawnMarker(TestWorld, TEXT("Exit"), FVector(3000.0f, 0.0f, 0.0f));
	AOutbreakMissionMarker* Final = SpawnMarker(TestWorld, TEXT("Final"), FVector(-3000.0f, 0.0f, 0.0f));
	AOutbreakCombatTestTarget* Target = TestWorld.Spawn<AOutbreakCombatTestTarget>(FVector(0.0f, 3000.0f, 0.0f));
	Target->Tags.Add(TEXT("Clear"));

	UOutbreakMissionDefinition* Definition = MakeDefinition();
	AddReach(Definition, TEXT("Reach the exit"), TEXT("Exit"), TEXT("Reach"), EA::AtMissionStart);
	UOutbreakEliminateTargetObjective* Clear = AddObjective<UOutbreakEliminateTargetObjective>(Definition, TEXT("Clear the area"), TEXT("Clear"), EA::AtMissionStart);
	Clear->TargetTag = TEXT("Clear");
	AddReach(Definition, TEXT("Find the cache"), TEXT("NoSuchMarker"), TEXT("Bonus"), EA::AtMissionStart, /*bOptional=*/ true);
	UOutbreakEliminateTargetObjective* FollowUp = AddObjective<UOutbreakEliminateTargetObjective>(Definition, TEXT("Destroy the cache"),
		TEXT("BonusFollowUp"), EA::AfterPrerequisites, /*bOptional=*/ true);
	FollowUp->TargetTag = TEXT("Cache");
	FollowUp->Prerequisites = { TEXT("Bonus") };
	AddReach(Definition, TEXT("Regroup"), TEXT("Final"), TEXT("Regroup"), EA::AfterPrerequisites)->Prerequisites = { TEXT("Reach"), TEXT("Clear") };

	UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
	if (!TestTrue(TEXT("Mission initialized"), Mission->InitializeMission(Definition)))
	{
		return false;
	}
	Mission->RegisterPlayer(Player);

	AddExpectedError(TEXT("no AOutbreakMissionMarker with ID 'NoSuchMarker'"), EAutomationExpectedErrorFlags::Contains, 1);
	Mission->StartMission();
	auto State = [Mission](int32 Index) { return Mission->GetObjective(Index)->GetState(); };
	TestTrue(TEXT("Three objectives active at once"), State(0) == ES::Active && State(1) == ES::Active && State(2) == ES::Active);
	TestTrue(TEXT("Final waits for its prerequisites"), State(4) == ES::Inactive);
	TestEqual(TEXT("Current = first active required"), Mission->GetCurrentObjectiveIndex(), 0);

	const FOutbreakMissionStatus Before = Mission->BuildStatus();
	Mission->UpdateMission();
	TestTrue(TEXT("Optional objective failed"), State(2) == ES::Failed);
	TestTrue(TEXT("Its follow-up is cancelled"), State(3) == ES::Failed);
	TestTrue(TEXT("Optional failure does not fail the mission"), Mission->GetMissionState() == EOutbreakMissionState::InProgress);

	const TArray<FText> Notes = OutbreakMissionText::BuildObjectiveNotifications(Before, Mission->BuildStatus());
	TestTrue(TEXT("Optional failure is announced"), Notes.ContainsByPredicate([](const FText& Note)
	{
		return Note.ToString().Contains(TEXT("Optional objective failed: Find the cache"));
	}));

	Target->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	Mission->UpdateMission();
	TestTrue(TEXT("Clear completed"), State(1) == ES::Completed);
	TestTrue(TEXT("Final still waits for Reach"), State(4) == ES::Inactive);

	Player->SetActorLocation(Exit->GetActorLocation());
	Mission->UpdateMission();
	TestTrue(TEXT("Both prerequisites done: final active"), State(4) == ES::Active);
	TestEqual(TEXT("Current moves to the final objective"), Mission->GetCurrentObjectiveIndex(), 4);

	Player->SetActorLocation(Final->GetActorLocation());
	Mission->UpdateMission();
	TestTrue(TEXT("All required objectives done: complete"), Mission->GetMissionState() == EOutbreakMissionState::Completed);

	const FOutbreakMissionResults& Results = Mission->GetMissionResults();
	TestEqual(TEXT("Required 3 / 3"), Results.RequiredObjectivesCompleted, 3);
	TestEqual(TEXT("Required count"), Results.RequiredObjectiveCount, 3);
	TestEqual(TEXT("Optional 0 / 2"), Results.OptionalObjectivesCompleted, 0);
	TestEqual(TEXT("Optional count"), Results.OptionalObjectiveCount, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakObjectiveSequentialDefaultTest, "Outbreak.Mission.Dependencies.SequentialByDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Phase 6 missions (no dependency settings) still run one objective after another. */
bool FOutbreakObjectiveSequentialDefaultTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	SpawnMarker(TestWorld, TEXT("A"), FVector::ZeroVector);
	AOutbreakMissionMarker* B = SpawnMarker(TestWorld, TEXT("B"), FVector(5000.0f, 0.0f, 0.0f));

	UOutbreakMissionDefinition* Definition = MakeDefinition();
	AddReach(Definition, TEXT("First"), TEXT("A"));
	AddReach(Definition, TEXT("Second"), TEXT("B"));

	UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
	Mission->InitializeMission(Definition);
	Mission->RegisterPlayer(Player);
	Mission->StartMission();
	TestTrue(TEXT("Only the first is active"), Mission->GetObjective(1)->GetState() == EOutbreakObjectiveState::Inactive);

	Mission->UpdateMission();
	TestTrue(TEXT("Second follows the first"), Mission->GetObjective(1)->GetState() == EOutbreakObjectiveState::Active);
	Player->SetActorLocation(B->GetActorLocation());
	Mission->UpdateMission();
	TestTrue(TEXT("Completed"), Mission->GetMissionState() == EOutbreakMissionState::Completed);
	return true;
}

// --- Objective types -----------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakDestroyObjectTest, "Outbreak.Mission.Objectives.DestroyObject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakDestroyObjectTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakDestructibleObject* RelayA = TestWorld.Spawn<AOutbreakDestructibleObject>(FVector(1000.0f, 0.0f, 0.0f));
	AOutbreakDestructibleObject* RelayB = TestWorld.Spawn<AOutbreakDestructibleObject>(FVector(-1000.0f, 0.0f, 0.0f));
	RelayA->Tags.Add(TEXT("Relay"));
	RelayB->Tags.Add(TEXT("Relay"));

	UOutbreakMissionDefinition* Definition = MakeDefinition();
	UOutbreakDestroyObjectObjective* Destroy = AddObjective<UOutbreakDestroyObjectObjective>(Definition, TEXT("Destroy the relays"));
	TArray<FText> Errors;
	TestFalse(TEXT("No tag is invalid"), Definition->Validate(Errors));
	Destroy->TargetTag = TEXT("Relay");

	UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
	TestTrue(TEXT("Initialized"), Mission->InitializeMission(Definition));
	Mission->RegisterPlayer(Player);
	Mission->StartMission();

	RelayA->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	Mission->UpdateMission();
	TestTrue(TEXT("Destroyed object is hidden"), RelayA->IsDestroyed() && RelayA->IsHidden());
	TestEqual(TEXT("Progress"), Mission->BuildStatus().ObjectiveProgress.ToString(), FString(TEXT("1 / 2")));

	RelayB->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	Mission->UpdateMission();
	TestTrue(TEXT("All destroyed: complete"), Mission->GetMissionState() == EOutbreakMissionState::Completed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakProtectLocationTest, "Outbreak.Mission.Objectives.ProtectLocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Holding for the duration completes; hostiles occupying the zone too long, or a protected object destroyed, fails. */
bool FOutbreakProtectLocationTest::RunTest(const FString& Parameters)
{
	auto MakeProtect = [](FName ProtectedTag)
	{
		UOutbreakMissionDefinition* Definition = MakeDefinition();
		UOutbreakProtectLocationObjective* Protect = AddObjective<UOutbreakProtectLocationObjective>(Definition, TEXT("Hold the uplink"));
		Protect->MarkerId = TEXT("Uplink");
		Protect->Radius = 1000.0f;
		Protect->Duration = 30.0f;
		Protect->MaxHostileTime = 5.0f;
		Protect->ProtectedTag = ProtectedTag;
		return Definition;
	};

	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		SpawnMarker(TestWorld, TEXT("Uplink"), FVector::ZeroVector);
		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(MakeProtect(NAME_None));
		Mission->RegisterPlayer(Player);
		Mission->StartMission();

		Mission->AdvanceMission(10.0f);
		TestEqual(TEXT("Time left shown"), Mission->BuildStatus().ObjectiveProgress.ToString(), FString(TEXT("0:20 left")));
		TestEqual(TEXT("Mission time advances"), Mission->GetMissionTime(), 10.0f);

		AOutbreakEnemyCharacter* Enemy = TestWorld.Spawn<AOutbreakEnemyCharacter>(FVector(500.0f, 0.0f, 0.0f));
		Mission->AdvanceMission(3.0f);
		TestTrue(TEXT("Contested"), Cast<UOutbreakProtectLocationObjective>(Mission->GetObjective(0))->IsContested());
		TestTrue(TEXT("Contested but not lost yet"), Mission->GetMissionState() == EOutbreakMissionState::InProgress);

		Enemy->SetActorLocation(FVector(5000.0f, 0.0f, 0.0f));
		Mission->AdvanceMission(1.0f);
		Enemy->SetActorLocation(FVector(500.0f, 0.0f, 0.0f));
		Mission->AdvanceMission(3.0f);
		TestTrue(TEXT("A break resets the hostile time"), Mission->GetMissionState() == EOutbreakMissionState::InProgress);

		Enemy->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
		Mission->AdvanceMission(14.0f);
		TestTrue(TEXT("Dead hostiles do not count; held 31 s: complete"), Mission->GetMissionState() == EOutbreakMissionState::Completed);
	}
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		SpawnMarker(TestWorld, TEXT("Uplink"), FVector::ZeroVector);
		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(MakeProtect(NAME_None));
		Mission->RegisterPlayer(Player);
		Mission->StartMission();

		TestWorld.Spawn<AOutbreakEnemyCharacter>(FVector(500.0f, 0.0f, 0.0f));
		Mission->AdvanceMission(3.0f);
		Mission->AdvanceMission(3.0f);
		TestTrue(TEXT("Hostiles held the zone: failed"), Mission->GetMissionState() == EOutbreakMissionState::Failed);
	}
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		SpawnMarker(TestWorld, TEXT("Uplink"), FVector::ZeroVector);
		AOutbreakDestructibleObject* Antenna = TestWorld.Spawn<AOutbreakDestructibleObject>(FVector(200.0f, 0.0f, 0.0f));
		Antenna->Tags.Add(TEXT("Antenna"));
		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(MakeProtect(TEXT("Antenna")));
		Mission->RegisterPlayer(Player);
		Mission->StartMission();

		Antenna->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
		Mission->AdvanceMission(1.0f);
		TestTrue(TEXT("Protected object destroyed: failed"), Mission->GetMissionState() == EOutbreakMissionState::Failed);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakRetrieveItemTest, "Outbreak.Mission.Objectives.RetrieveItem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Carrying the item (by anyone in the squad) completes it; with a delivery marker it must also be brought there. */
bool FOutbreakRetrieveItemTest::RunTest(const FString& Parameters)
{
	UOutbreakItemDefinition* Drive = MakeMissionItem(TEXT("Data drive"));
	{
		FOutbreakTestWorld TestWorld;
		FSquad Squad;
		if (!TestTrue(TEXT("Squad"), Squad.Create(TestWorld, FVector(400.0f, 0.0f, 0.0f))))
		{
			return false;
		}
		UOutbreakMissionDefinition* Definition = MakeDefinition();
		UOutbreakRetrieveItemObjective* Retrieve = AddObjective<UOutbreakRetrieveItemObjective>(Definition, TEXT("Recover the drive"));
		TArray<FText> Errors;
		TestFalse(TEXT("No item is invalid"), Definition->Validate(Errors));
		Retrieve->Item = Drive;

		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(Definition);
		Mission->RegisterPlayer(Squad.Player);
		Mission->StartMission();

		Mission->UpdateMission();
		TestEqual(TEXT("Nothing carried"), Mission->BuildStatus().ObjectiveProgress.ToString(), FString(TEXT("0 / 1 recovered")));
		Squad.Teammate->GetInventoryComponent()->AddItem(Drive, 1);
		Mission->UpdateMission();
		TestTrue(TEXT("A teammate carrying it counts"), Mission->GetMissionState() == EOutbreakMissionState::Completed);
	}
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		AOutbreakMissionMarker* Lab = SpawnMarker(TestWorld, TEXT("Lab"), FVector(4000.0f, 0.0f, 0.0f));
		UOutbreakMissionDefinition* Definition = MakeDefinition();
		UOutbreakRetrieveItemObjective* Retrieve = AddObjective<UOutbreakRetrieveItemObjective>(Definition, TEXT("Bring the drive to the lab"));
		Retrieve->Item = Drive;
		Retrieve->DeliveryMarkerId = TEXT("Lab");

		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(Definition);
		Mission->RegisterPlayer(Player);
		Mission->StartMission();

		Player->GetInventoryComponent()->AddItem(Drive, 1);
		Mission->UpdateMission();
		TestTrue(TEXT("Carried but not delivered"), Mission->GetMissionState() == EOutbreakMissionState::InProgress);
		TestEqual(TEXT("Distance to delivery"), Mission->BuildStatus().ObjectiveProgress.ToString(), FString(TEXT("deliver: 40 m")));

		Player->SetActorLocation(Lab->GetActorLocation());
		Mission->UpdateMission();
		TestTrue(TEXT("Delivered: complete"), Mission->GetMissionState() == EOutbreakMissionState::Completed);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakActivateEquipmentTest, "Outbreak.Mission.Objectives.ActivateEquipment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Equipment stays locked until its objective; a required item is checked and used up; activation completes it. */
bool FOutbreakActivateEquipmentTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	SpawnMarker(TestWorld, TEXT("Gate"), FVector(5000.0f, 0.0f, 0.0f));
	AOutbreakMissionEquipment* Terminal = TestWorld.Spawn<AOutbreakMissionEquipment>(FVector(100.0f, 0.0f, 0.0f));
	Terminal->Tags.Add(TEXT("Terminal"));

	UOutbreakMissionDefinition* Definition = MakeDefinition();
	AddReach(Definition, TEXT("Reach the gate"), TEXT("Gate"));
	UOutbreakActivateEquipmentObjective* Activate = AddObjective<UOutbreakActivateEquipmentObjective>(Definition, TEXT("Open the gate"));
	Activate->EquipmentTag = TEXT("Terminal");

	UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
	Mission->InitializeMission(Definition);
	Mission->RegisterPlayer(Player);
	Mission->StartMission();

	TestTrue(TEXT("Locked before its objective"), Terminal->IsLocked());
	TestFalse(TEXT("Cannot interact while locked"), IOutbreakInteractable::Execute_CanInteract(Terminal, Player));

	Player->SetActorLocation(FVector(5000.0f, 0.0f, 0.0f));
	Mission->UpdateMission();
	TestFalse(TEXT("Unlocked when the objective activates"), Terminal->IsLocked());

	IOutbreakInteractable::Execute_Interact(Terminal, Player);
	Mission->UpdateMission();
	TestTrue(TEXT("Activated: mission complete"), Terminal->IsActivated() && Mission->GetMissionState() == EOutbreakMissionState::Completed);
	TestFalse(TEXT("No second activation"), Terminal->Activate(Player));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEquipmentRequiredItemTest, "Outbreak.Mission.Objectives.EquipmentRequiredItem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Equipment that needs an item says so, refuses without it, and uses it up. */
bool FOutbreakEquipmentRequiredItemTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld KeyWorld;
	AOutbreakPlayerCharacter* KeyPlayer = KeyWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakMissionEquipment* Door = KeyWorld.Spawn<AOutbreakMissionEquipment>(FVector(100.0f, 0.0f, 0.0f));
	UOutbreakItemDefinition* Keycard = MakeMissionItem(TEXT("Keycard"));
	Door->SetRequiredItem(Keycard, /*bConsume=*/ true);
	Door->SetLocked(false);

	TestTrue(TEXT("Focusable without the item"), IOutbreakInteractable::Execute_CanInteract(Door, KeyPlayer));
	TestEqual(TEXT("Prompt names the item"), IOutbreakInteractable::Execute_GetInteractionPrompt(Door, KeyPlayer).ToString(), FString(TEXT("Requires Keycard")));
	IOutbreakInteractable::Execute_Interact(Door, KeyPlayer);
	TestFalse(TEXT("Not activated without the item"), Door->IsActivated());

	KeyPlayer->GetInventoryComponent()->AddItem(Keycard, 1);
	IOutbreakInteractable::Execute_Interact(Door, KeyPlayer);
	TestTrue(TEXT("Activated with the item"), Door->IsActivated());
	TestEqual(TEXT("Item used up"), KeyPlayer->GetInventoryComponent()->GetItemCount(Keycard), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakExtractionTest, "Outbreak.Mission.Objectives.Extraction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Players and standing teammates must hold the zone for the extraction time; downed soldiers are left behind. */
bool FOutbreakExtractionTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	FSquad Squad;
	if (!TestTrue(TEXT("Squad"), Squad.Create(TestWorld, FVector(3000.0f, 0.0f, 0.0f))))
	{
		return false;
	}
	AOutbreakMissionMarker* Zone = SpawnMarker(TestWorld, TEXT("LZ"), FVector::ZeroVector);

	UOutbreakMissionDefinition* Definition = MakeDefinition();
	UOutbreakExtractionObjective* Extraction = AddObjective<UOutbreakExtractionObjective>(Definition, TEXT("Extract"));
	Extraction->MarkerId = TEXT("LZ");
	Extraction->Radius = 600.0f;
	Extraction->ExtractionTime = 5.0f;

	UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
	Mission->InitializeMission(Definition);
	Mission->RegisterPlayer(Squad.Player);
	Mission->StartMission();

	Mission->AdvanceMission(10.0f);
	TestEqual(TEXT("Waiting for the teammate"), Mission->BuildStatus().ObjectiveProgress.ToString(), FString(TEXT("1 / 2 in zone")));
	TestTrue(TEXT("Teammate outside: no extraction"), Mission->GetMissionState() == EOutbreakMissionState::InProgress);

	Squad.Teammate->SetActorLocation(Zone->GetActorLocation() + FVector(300.0f, 0.0f, 0.0f));
	Mission->AdvanceMission(1.0f);
	TestTrue(TEXT("Countdown starts on arrival"), Mission->BuildStatus().ObjectiveProgress.ToString().Contains(TEXT("extracting in 0:05")));
	Mission->AdvanceMission(4.0f);
	TestTrue(TEXT("Not yet"), Mission->GetMissionState() == EOutbreakMissionState::InProgress);

	Squad.Player->SetActorLocation(FVector(2000.0f, 0.0f, 0.0f));
	Mission->AdvanceMission(1.0f);
	Squad.Player->SetActorLocation(FVector::ZeroVector);
	Mission->AdvanceMission(1.0f);
	Mission->AdvanceMission(4.0f);
	TestTrue(TEXT("Leaving reset the countdown"), Mission->GetMissionState() == EOutbreakMissionState::InProgress);
	Mission->AdvanceMission(1.0f);
	TestTrue(TEXT("Held 5 s: extracted"), Mission->GetMissionState() == EOutbreakMissionState::Completed);

	const FOutbreakMissionResults& Results = Mission->GetMissionResults();
	TestEqual(TEXT("Both soldiers in the report"), Results.Squad.Num(), 2);
	TestTrue(TEXT("Everyone extracted"), Results.Squad.Num() == 2 && Results.Squad[0].bExtracted && Results.Squad[1].bExtracted);
	TestTrue(TEXT("Player listed first"), Results.Squad.Num() > 0 && Results.Squad[0].bIsPlayer);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakExtractionLeftBehindTest, "Outbreak.Mission.Objectives.ExtractionLeavesDownedBehind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** A downed teammate is not required to extract and is reported as left behind. */
bool FOutbreakExtractionLeftBehindTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld DownWorld;
	FSquad Downed;
	if (!TestTrue(TEXT("Squad"), Downed.Create(DownWorld, FVector(3000.0f, 0.0f, 0.0f))))
	{
		return false;
	}
	SpawnMarker(DownWorld, TEXT("LZ"), FVector::ZeroVector);
	UOutbreakMissionSubsystem* DownMission = DownWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
	UOutbreakMissionDefinition* Instant = MakeDefinition();
	UOutbreakExtractionObjective* InstantExtraction = AddObjective<UOutbreakExtractionObjective>(Instant, TEXT("Extract"));
	InstantExtraction->MarkerId = TEXT("LZ");
	InstantExtraction->ExtractionTime = 0.0f;
	DownMission->InitializeMission(Instant);
	DownMission->RegisterPlayer(Downed.Player);
	DownMission->StartMission();

	Downed.Teammate->GetHealthComponent()->ConfigureIncapacitation(true, 30.0f, 0);
	Downed.Teammate->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	DownMission->UpdateMission();
	TestTrue(TEXT("Downed teammate does not block extraction"), DownMission->GetMissionState() == EOutbreakMissionState::Completed);
	const FOutbreakMissionResults& DownResults = DownMission->GetMissionResults();
	TestTrue(TEXT("Downed teammate reported, not extracted"), DownResults.Squad.Num() == 2
		&& !DownResults.Squad[1].bExtracted && DownResults.Squad[1].Status == EOutbreakSquadMemberStatus::Incapacitated);
	TestTrue(TEXT("Summary says left behind"), OutbreakMissionText::BuildSummary(DownResults).ToString().Contains(TEXT("Hawk: Down, left behind")));
	return true;
}

// --- Flow ----------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakBriefingDeploymentTest, "Outbreak.Mission.Flow.BriefingAndDeployment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** The mission waits in the briefing until every player has deployed at the starting location. */
bool FOutbreakBriefingDeploymentTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* PlayerA = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakPlayerCharacter* PlayerB = TestWorld.Spawn<AOutbreakPlayerCharacter>(FVector(0.0f, 500.0f, 0.0f));
	AOutbreakMissionMarker* Start = SpawnMarker(TestWorld, TEXT("Insertion"), FVector(2000.0f, 2000.0f, 0.0f));
	Start->SetActorRotation(FRotator(0.0f, 90.0f, 0.0f));
	SpawnMarker(TestWorld, TEXT("Far"), FVector(90000.0f, 0.0f, 0.0f));

	UOutbreakMissionDefinition* Definition = MakeDefinition();
	Definition->DeploymentMarkerId = TEXT("Insertion");
	Definition->DeploymentSpacing = 150.0f;
	AddReach(Definition, TEXT("Go far"), TEXT("Far"));

	UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
	Mission->InitializeMission(Definition);
	Mission->RegisterPlayer(PlayerA);
	Mission->RegisterPlayer(PlayerB);
	Mission->BeginBriefing();
	TestTrue(TEXT("Briefing"), Mission->GetMissionState() == EOutbreakMissionState::Briefing);
	TestTrue(TEXT("Briefing status published"), Mission->BuildStatus().State == EOutbreakMissionState::Briefing);

	TestTrue(TEXT("Moved to the starting location"), Mission->MovePawnToDeployment(PlayerA));
	TestTrue(TEXT("First player at the marker"), PlayerA->GetActorLocation().Equals(Start->GetActorLocation(), 1.0f));
	TestEqual(TEXT("Facing the marker's direction"), PlayerA->GetActorRotation().Yaw, 90.0, 0.1);
	Mission->NotifyPlayerDeployed(PlayerA);
	TestTrue(TEXT("Waits for the second player"), Mission->GetMissionState() == EOutbreakMissionState::Briefing);
	TestTrue(TEXT("A deployed"), Mission->IsPlayerDeployed(PlayerA) && !Mission->IsPlayerDeployed(PlayerB));

	Mission->MovePawnToDeployment(PlayerB);
	TestEqual(TEXT("Second player beside the first"), FVector::Dist(PlayerB->GetActorLocation(), Start->GetActorLocation()), 150.0, 1.0);
	Mission->NotifyPlayerDeployed(PlayerB);
	TestTrue(TEXT("Everyone deployed: mission starts"), Mission->GetMissionState() == EOutbreakMissionState::InProgress);
	TestTrue(TEXT("First objective active"), Mission->GetObjective(0)->GetState() == EOutbreakObjectiveState::Active);

	// No marker: players stay where they spawned.
	UOutbreakMissionDefinition* NoMarker = MakeDefinition();
	AddReach(NoMarker, TEXT("Go far"), TEXT("Far"));
	Mission->InitializeMission(NoMarker);
	TestFalse(TEXT("Without a deployment marker nobody moves"), Mission->MovePawnToDeployment(PlayerA));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakMissionCleanupTest, "Outbreak.Mission.Flow.CleanupAndResults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Ending deactivates running objectives and stops reacting to players; re-initializing starts clean; results are final. */
bool FOutbreakMissionCleanupTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	FSquad Squad;
	if (!TestTrue(TEXT("Squad"), Squad.Create(TestWorld, FVector(300.0f, 0.0f, 0.0f))))
	{
		return false;
	}
	SpawnMarker(TestWorld, TEXT("Uplink"), FVector::ZeroVector);
	SpawnMarker(TestWorld, TEXT("Far"), FVector(90000.0f, 0.0f, 0.0f));

	UOutbreakMissionDefinition* Definition = MakeDefinition();
	Definition->MissionId = TEXT("M01");
	AddReach(Definition, TEXT("Go far"), TEXT("Far"));
	UOutbreakProtectLocationObjective* Protect = AddObjective<UOutbreakProtectLocationObjective>(Definition, TEXT("Hold"),
		NAME_None, EOutbreakObjectiveActivation::AtMissionStart, /*bOptional=*/ true);
	Protect->MarkerId = TEXT("Uplink");
	Protect->Duration = 600.0f;

	UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
	Mission->InitializeMission(Definition);
	Mission->RegisterPlayer(Squad.Player);
	Mission->StartMission();
	Mission->AdvanceMission(42.0f);

	TestWorld.Spawn<AOutbreakEnemyCharacter>(FVector(100.0f, 0.0f, 0.0f));
	Mission->FailMission(FText::FromString(TEXT("Aborted")));
	TestTrue(TEXT("Failed"), Mission->GetMissionState() == EOutbreakMissionState::Failed);

	UOutbreakProtectLocationObjective* RunningProtect = Cast<UOutbreakProtectLocationObjective>(Mission->GetObjective(1));
	Mission->AdvanceMission(1000.0f);
	TestFalse(TEXT("Ended mission no longer evaluates"), RunningProtect->IsContested());
	TestEqual(TEXT("Mission time stops at the end"), Mission->GetMissionTime(), 42.0f);

	const FOutbreakMissionResults& Results = Mission->GetMissionResults();
	TestTrue(TEXT("Results are final"), Results.IsFinal() && Results.Outcome == EOutbreakMissionState::Failed);
	TestTrue(TEXT("Mission ID"), Results.MissionId == FName(TEXT("M01")));
	TestEqual(TEXT("Duration"), Results.Duration, 42.0f);
	TestEqual(TEXT("Reason"), Results.ResultText.ToString(), FString(TEXT("Aborted")));
	TestEqual(TEXT("Squad of two reported"), Results.Squad.Num(), 2);

	Squad.Player->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	TestEqual(TEXT("Player events no longer change the outcome"), Mission->GetMissionResults().ResultText.ToString(), FString(TEXT("Aborted")));

	const FString Summary = OutbreakMissionText::BuildSummary(Results).ToString();
	TestTrue(TEXT("Summary shows the time"), Summary.Contains(TEXT("Mission time: 0:42")));
	TestTrue(TEXT("Summary counts objectives"), Summary.Contains(TEXT("0 / 1 required, 0 / 1 optional")));
	TestEqual(TEXT("Title"), OutbreakMissionText::BuildResultTitle(Results).ToString(), FString(TEXT("MISSION FAILED")));

	TestTrue(TEXT("Re-initialized"), Mission->InitializeMission(Definition));
	TestTrue(TEXT("Clean state"), Mission->GetMissionState() == EOutbreakMissionState::NotStarted && Mission->GetMissionTime() == 0.0f);
	TestFalse(TEXT("Results reset"), Mission->GetMissionResults().IsFinal());
	TestTrue(TEXT("Fresh objectives"), Mission->GetObjective(0)->GetState() == EOutbreakObjectiveState::Inactive);
	return true;
}

// --- Squad and equipment selection ---------------------------------------------------------------

namespace OutbreakMissionFrameworkTests
{
	UOutbreakSquadConfiguration* MakeRosterConfiguration(UOutbreakLoadoutDefinition* Loadout)
	{
		UOutbreakSquadConfiguration* Configuration = NewObject<UOutbreakSquadConfiguration>(GetTransientPackage());
		Configuration->SquadSize = 4;
		for (const TCHAR* Name : { TEXT("Alpha"), TEXT("Bravo"), TEXT("Charlie") })
		{
			Configuration->Teammates.AddDefaulted_GetRef().DisplayName = FText::FromString(Name);
		}
		Configuration->FillerTeammate.DisplayName = FText::FromString(TEXT("Recruit"));
		if (Loadout)
		{
			Configuration->AvailableLoadouts.Add(Loadout);
		}
		return Configuration;
	}

	UOutbreakLoadoutDefinition* MakeLoadout(UOutbreakItemDefinition* Item)
	{
		UOutbreakLoadoutDefinition* Loadout = NewObject<UOutbreakLoadoutDefinition>(GetTransientPackage());
		Loadout->DisplayName = FText::FromString(TEXT("Demolitions"));
		FOutbreakItemAmount& Entry = Loadout->Loadout.Items.AddDefaulted_GetRef();
		Entry.Item = Item;
		Entry.Count = 2;
		return Loadout;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadSelectionRulesTest, "Outbreak.Squad.Selection.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakSquadSelectionRulesTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakSquadSelection;
	UOutbreakLoadoutDefinition* Loadout = MakeLoadout(MakeMissionItem(TEXT("Charge")));
	UOutbreakSquadConfiguration* Configuration = MakeRosterConfiguration(Loadout);
	FOutbreakSquadSizeLimits Limits;
	Limits.MinSquadSize = 2;
	Limits.DefaultSquadSize = 4;
	Limits.MaxSquadSize = 6;

	FOutbreakSquadSelection Selection = MakeDefault(Configuration, Limits);
	TestEqual(TEXT("Configuration size"), Selection.SquadSize, 4);
	TestTrue(TEXT("Roster in order"), Selection.TeammateRosterIndices == TArray<int32>({ 0, 1, 2 }));
	TestEqual(TEXT("A loadout entry per member"), Selection.MemberLoadouts.Num(), 4);
	TArray<FText> Errors;
	TestTrue(TEXT("Default is valid"), Validate(Configuration, Limits, Selection, Errors));

	SetSquadSize(Configuration, Limits, Selection, 6);
	TestTrue(TEXT("Grown with fillers"), Selection.TeammateRosterIndices == TArray<int32>({ 0, 1, 2, INDEX_NONE, INDEX_NONE }));
	SetSquadSize(Configuration, Limits, Selection, 9);
	TestEqual(TEXT("Clamped to the maximum"), Selection.SquadSize, 6);
	SetSquadSize(Configuration, Limits, Selection, 1);
	TestEqual(TEXT("Clamped to the minimum"), Selection.SquadSize, 2);
	TestTrue(TEXT("Shrunk from the end"), Selection.TeammateRosterIndices == TArray<int32>({ 0 }));

	SetSquadSize(Configuration, Limits, Selection, 3);
	TestTrue(TEXT("Next unused soldier"), Selection.TeammateRosterIndices == TArray<int32>({ 0, 1 }));
	CycleTeammate(Configuration, Selection, 0, 1);
	TestEqual(TEXT("Cycling skips soldiers in other slots"), Selection.TeammateRosterIndices[0], 2);
	CycleTeammate(Configuration, Selection, 0, 1);
	TestEqual(TEXT("Then the filler"), Selection.TeammateRosterIndices[0], static_cast<int32>(INDEX_NONE));
	CycleTeammate(Configuration, Selection, 0, -1);
	TestEqual(TEXT("And back"), Selection.TeammateRosterIndices[0], 2);
	TestEqual(TEXT("Names from the roster"), OutbreakMissionText::GetTeammateName(Configuration, Selection, 0).ToString(), FString(TEXT("Charlie")));

	CycleLoadout(Configuration, Selection, 0, 1);
	TestTrue(TEXT("Player takes the loadout"), GetLoadout(Selection, 0) == Loadout);
	TestEqual(TEXT("Loadout name"), OutbreakMissionText::GetLoadoutName(Selection, 0).ToString(), FString(TEXT("Demolitions")));
	CycleLoadout(Configuration, Selection, 0, 1);
	TestNull(TEXT("Wraps to default equipment"), GetLoadout(Selection, 0));

	FOutbreakSquadSelection Duplicate = Selection;
	Duplicate.TeammateRosterIndices = { 1, 1 };
	Errors.Reset();
	TestFalse(TEXT("Same soldier twice rejected"), Validate(Configuration, Limits, Duplicate, Errors));

	FOutbreakSquadSelection Foreign = Selection;
	Foreign.MemberLoadouts[1] = MakeLoadout(MakeMissionItem(TEXT("Other")));
	Errors.Reset();
	TestFalse(TEXT("Loadout not offered rejected"), Validate(Configuration, Limits, Foreign, Errors));

	FOutbreakSquadSelection BadSlots = Selection;
	BadSlots.TeammateRosterIndices = { 0 };
	Errors.Reset();
	TestFalse(TEXT("Slot count must match the size"), Validate(Configuration, Limits, BadSlots, Errors));

	FOutbreakSquadSelection TooBig = MakeDefault(Configuration, Limits);
	TooBig.SquadSize = 7;
	TooBig.TeammateRosterIndices.SetNum(6);
	Errors.Reset();
	TestFalse(TEXT("Size above the limit rejected"), Validate(Configuration, Limits, TooBig, Errors));

	const FOutbreakSquadSelection Generic = MakeDefault(nullptr, Limits);
	TestTrue(TEXT("No configuration: default size of fillers"), Generic.SquadSize == 4
		&& Generic.TeammateRosterIndices == TArray<int32>({ INDEX_NONE, INDEX_NONE, INDEX_NONE }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadSelectionInitTest, "Outbreak.Squad.Selection.InitializeWithSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** The squad is built from the chosen size, roster soldiers and loadouts; an invalid selection falls back to the default. */
bool FOutbreakSquadSelectionInitTest::RunTest(const FString& Parameters)
{
	UOutbreakItemDefinition* Charge = MakeMissionItem(TEXT("Charge"));
	UOutbreakLoadoutDefinition* Loadout = MakeLoadout(Charge);
	UOutbreakSquadConfiguration* Configuration = MakeRosterConfiguration(Loadout);
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerState* PlayerState = TestWorld.Spawn<AOutbreakPlayerState>();
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		UOutbreakSquadComponent* Squad = PlayerState->GetSquadComponent();

		FOutbreakSquadSelection Selection;
		Selection.SquadSize = 3;
		Selection.TeammateRosterIndices = { 2, INDEX_NONE };
		Selection.MemberLoadouts = { Loadout, nullptr, Loadout };

		TestTrue(TEXT("Squad created"), Squad->InitializeSquadWithSelection(Configuration, Player, Selection));
		const TArray<UOutbreakSquadMemberComponent*> Teammates = Squad->GetTeammates();
		if (!TestEqual(TEXT("Chosen size"), Squad->GetMemberCount(), 3) || Teammates.Num() != 2)
		{
			return false;
		}
		TestEqual(TEXT("Chosen roster soldier"), Teammates[0]->GetDisplayName().ToString(), FString(TEXT("Charlie")));
		TestEqual(TEXT("Filler"), Teammates[1]->GetDisplayName().ToString(), FString(TEXT("Recruit")));
		TestEqual(TEXT("Player loadout applied"), Player->GetInventoryComponent()->GetItemCount(Charge), 2);
		TestEqual(TEXT("Teammate without loadout keeps defaults"),
			Teammates[0]->GetOwner()->FindComponentByClass<UOutbreakInventoryComponent>()->GetItemCount(Charge), 0);
		TestEqual(TEXT("Second teammate loadout applied"),
			Teammates[1]->GetOwner()->FindComponentByClass<UOutbreakInventoryComponent>()->GetItemCount(Charge), 2);

		Squad->CloseLoadoutSelection();
		TestFalse(TEXT("After deployment equipment is fixed"), Squad->SelectMemberLoadout(Teammates[0], Loadout));
	}
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerState* PlayerState = TestWorld.Spawn<AOutbreakPlayerState>();
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		UOutbreakSquadComponent* Squad = PlayerState->GetSquadComponent();

		FOutbreakSquadSelection Invalid;
		Invalid.SquadSize = 3;
		Invalid.TeammateRosterIndices = { 1, 1 };
		Squad->InitializeSquadWithSelection(Configuration, Player, Invalid);
		TestEqual(TEXT("Invalid selection: default squad of the configuration"), Squad->GetMemberCount(), 4);
		TestEqual(TEXT("Default roster order"), Squad->GetTeammates()[0]->GetDisplayName().ToString(), FString(TEXT("Alpha")));
	}
	return true;
}

// --- UI text -------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakMissionTextTest, "Outbreak.Mission.UI.ObjectiveDisplayAndNotifications",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakMissionTextTest::RunTest(const FString& Parameters)
{
	using ES = EOutbreakObjectiveState;
	auto Objective = [](const TCHAR* Description, ES State, bool bOptional = false, bool bHidden = false, const TCHAR* Progress = TEXT(""))
	{
		FOutbreakObjectiveStatus Entry;
		Entry.Description = FText::FromString(Description);
		Entry.State = State;
		Entry.bOptional = bOptional;
		Entry.bHidden = bHidden;
		Entry.Progress = FText::FromString(Progress);
		return Entry;
	};

	FOutbreakMissionStatus Previous;
	Previous.State = EOutbreakMissionState::InProgress;
	Previous.MissionName = FText::FromString(TEXT("Nightfall"));
	Previous.Objectives = {
		Objective(TEXT("Reach the LZ"), ES::Active),
		Objective(TEXT("Rescue the pilot"), ES::Active, /*bOptional=*/ true),
		Objective(TEXT("Secret"), ES::Inactive, false, /*bHidden=*/ true),
		Objective(TEXT("Destroy the relay"), ES::Inactive),
	};

	FOutbreakMissionStatus Current = Previous;
	Current.Objectives[0].State = ES::Completed;
	Current.Objectives[3] = Objective(TEXT("Destroy the relay"), ES::Active, false, false, TEXT("0 / 2"));

	const FString Compact = OutbreakMissionText::BuildObjectiveList(Current, false).ToString();
	TestEqual(TEXT("Compact: active only, required first"), Compact,
		FString(TEXT("[ ] Destroy the relay  (0 / 2)\n[ ] (Optional) Rescue the pilot")));

	const FString Expanded = OutbreakMissionText::BuildObjectiveList(Current, true).ToString();
	TestTrue(TEXT("Expanded shows completed"), Expanded.Contains(TEXT("[x] Reach the LZ")));
	TestFalse(TEXT("Hidden objective not revealed"), Expanded.Contains(TEXT("Secret")));

	const TArray<FText> Notes = OutbreakMissionText::BuildObjectiveNotifications(Previous, Current);
	TestEqual(TEXT("Two notifications"), Notes.Num(), 2);
	TestTrue(TEXT("Completion"), Notes.Num() == 2 && Notes[0].ToString() == TEXT("Objective complete: Reach the LZ"));
	TestTrue(TEXT("New objective"), Notes.Num() == 2 && Notes[1].ToString() == TEXT("New objective: Destroy the relay"));

	FOutbreakMissionStatus SecretCancelled = Current;
	SecretCancelled.Objectives[2].State = ES::Failed;
	TestEqual(TEXT("A hidden objective cancelled unseen stays secret"),
		OutbreakMissionText::BuildObjectiveNotifications(Current, SecretCancelled).Num(), 0);

	FOutbreakMissionStatus Briefing = Previous;
	Briefing.State = EOutbreakMissionState::Briefing;
	FOutbreakMissionStatus StillBriefing = Briefing;
	StillBriefing.MissionName = FText::FromString(TEXT("Renamed"));
	TestEqual(TEXT("No notifications during the briefing"), OutbreakMissionText::BuildObjectiveNotifications(Briefing, StillBriefing).Num(), 0);

	UOutbreakMissionDefinition* Definition = MakeDefinition();
	Definition->Description = FText::FromString(TEXT("Retake the outpost."));
	AddReach(Definition, TEXT("Reach the outpost"), TEXT("Outpost"));
	AddReach(Definition, TEXT("Find intel"), TEXT("Intel"), NAME_None, EOutbreakObjectiveActivation::AtMissionStart, true);
	AddReach(Definition, TEXT("Ambush"), TEXT("Ambush"))->bHidden = true;
	const FString BriefingText = OutbreakMissionText::BuildBriefing(Definition).ToString();
	TestTrue(TEXT("Briefing has the description"), BriefingText.Contains(TEXT("Retake the outpost.")));
	TestTrue(TEXT("Briefing lists objectives"), BriefingText.Contains(TEXT("- Reach the outpost")) && BriefingText.Contains(TEXT("- (Optional) Find intel")));
	TestFalse(TEXT("Briefing hides hidden objectives"), BriefingText.Contains(TEXT("Ambush")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
