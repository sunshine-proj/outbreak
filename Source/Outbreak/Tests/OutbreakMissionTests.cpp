#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/OutbreakCombatTestTarget.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Engine/World.h"
#include "Missions/OutbreakEliminateTargetObjective.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "Missions/OutbreakMissionMarker.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Missions/OutbreakReachLocationObjective.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Tests/OutbreakTestWorld.h"
#include "UObject/Package.h"

namespace OutbreakMissionTests
{
	const FName ExitMarker(TEXT("Exit"));
	const FName TargetTag(TEXT("MissionTarget"));

	/** Mission: reach the "Exit" marker, then eliminate everything tagged MissionTarget. No restart. */
	UOutbreakMissionDefinition* MakeMission()
	{
		UOutbreakMissionDefinition* Mission = NewObject<UOutbreakMissionDefinition>(GetTransientPackage());
		Mission->DisplayName = FText::FromString(TEXT("Test Mission"));
		Mission->RestartDelay = 0.0f;

		UOutbreakReachLocationObjective* Reach = NewObject<UOutbreakReachLocationObjective>(Mission);
		Reach->MarkerId = ExitMarker;
		Reach->Radius = 200.0f;
		Mission->Objectives.Add(Reach);

		UOutbreakEliminateTargetObjective* Eliminate = NewObject<UOutbreakEliminateTargetObjective>(Mission);
		Eliminate->TargetTag = TargetTag;
		Mission->Objectives.Add(Eliminate);
		return Mission;
	}

	AOutbreakCombatTestTarget* SpawnTaggedTarget(FOutbreakTestWorld& TestWorld, const FVector& Location)
	{
		AOutbreakCombatTestTarget* Target = TestWorld.Spawn<AOutbreakCombatTestTarget>(Location);
		if (Target)
		{
			Target->Tags.Add(TargetTag);
		}
		return Target;
	}
}

using namespace OutbreakMissionTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakMissionDefinitionTest, "Outbreak.Mission.Definition.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakMissionDefinitionTest::RunTest(const FString& Parameters)
{
	TArray<FText> Errors;
	TestTrue(TEXT("Valid mission"), MakeMission()->Validate(Errors));

	UOutbreakMissionDefinition* Empty = NewObject<UOutbreakMissionDefinition>(GetTransientPackage());
	Errors.Reset();
	TestFalse(TEXT("No objectives rejected"), Empty->Validate(Errors));

	UOutbreakMissionDefinition* NoMarker = MakeMission();
	Cast<UOutbreakReachLocationObjective>(NoMarker->Objectives[0])->MarkerId = NAME_None;
	Errors.Reset();
	TestFalse(TEXT("Reach without marker rejected"), NoMarker->Validate(Errors));

	UOutbreakMissionDefinition* NoTag = MakeMission();
	Cast<UOutbreakEliminateTargetObjective>(NoTag->Objectives[1])->TargetTag = NAME_None;
	Errors.Reset();
	TestFalse(TEXT("Eliminate without tag rejected"), NoTag->Validate(Errors));

	FOutbreakTestWorld TestWorld;
	UOutbreakMissionSubsystem* Subsystem = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
	if (TestNotNull(TEXT("Mission subsystem exists"), Subsystem))
	{
		TestFalse(TEXT("Invalid mission not initialized"), Subsystem->InitializeMission(Empty));
		Subsystem->StartMission();
		TestTrue(TEXT("Invalid mission does not start"), Subsystem->GetMissionState() == EOutbreakMissionState::NotStarted);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakMissionCompletionTest, "Outbreak.Mission.Flow.ObjectivesAndCompletion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Objectives run in order: reaching the marker activates the elimination, killing the targets completes the mission. */
bool FOutbreakMissionCompletionTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakMissionMarker* Marker = TestWorld.Spawn<AOutbreakMissionMarker>(FVector(3000.0f, 0.0f, 0.0f));
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakCombatTestTarget* TargetA = SpawnTaggedTarget(TestWorld, FVector(0.0f, 2000.0f, 0.0f));
	AOutbreakCombatTestTarget* TargetB = SpawnTaggedTarget(TestWorld, FVector(0.0f, -2000.0f, 0.0f));
	AOutbreakCombatTestTarget* Bystander = TestWorld.Spawn<AOutbreakCombatTestTarget>(FVector(0.0f, 4000.0f, 0.0f));
	UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
	if (!Marker || !Player || !TargetA || !TargetB || !Bystander || !TestNotNull(TEXT("Mission subsystem"), Mission))
	{
		return false;
	}
	Marker->SetMarkerId(ExitMarker);

	UOutbreakMissionDefinition* Definition = MakeMission();
	TestTrue(TEXT("Mission initialized"), Mission->InitializeMission(Definition));
	Mission->RegisterPlayer(Player);
	Mission->StartMission();

	TestTrue(TEXT("In progress"), Mission->GetMissionState() == EOutbreakMissionState::InProgress);
	TestEqual(TEXT("First objective"), Mission->GetCurrentObjectiveIndex(), 0);
	TestTrue(TEXT("Objectives are per-play copies"), Mission->GetCurrentObjective() != Definition->Objectives[0]);

	Mission->UpdateMission();
	TestEqual(TEXT("Still on the first objective far from the marker"), Mission->GetCurrentObjectiveIndex(), 0);
	TestEqual(TEXT("Distance shown"), Mission->BuildStatus().ObjectiveProgress.ToString(), FString(TEXT("30 m")));

	Player->SetActorLocation(Marker->GetActorLocation() + FVector(100.0f, 0.0f, 0.0f));
	Mission->UpdateMission();
	TestEqual(TEXT("Reached: second objective"), Mission->GetCurrentObjectiveIndex(), 1);

	Mission->UpdateMission();
	TestEqual(TEXT("Elimination progress"), Mission->BuildStatus().ObjectiveProgress.ToString(), FString(TEXT("0 / 2")));

	TargetA->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	Mission->UpdateMission();
	TestTrue(TEXT("One of two: still in progress"), Mission->GetMissionState() == EOutbreakMissionState::InProgress);
	TestEqual(TEXT("Progress counts the kill"), Mission->BuildStatus().ObjectiveProgress.ToString(), FString(TEXT("1 / 2")));

	TargetB->Destroy();
	Mission->UpdateMission();
	TestTrue(TEXT("Destroyed counts as eliminated: mission complete"), Mission->GetMissionState() == EOutbreakMissionState::Completed);
	TestTrue(TEXT("Untagged actors are not required"), Bystander->GetHealthComponent()->IsAlive());

	// A finished mission cannot fail afterwards.
	Player->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	TestTrue(TEXT("Completion is final"), Mission->GetMissionState() == EOutbreakMissionState::Completed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakMissionFailureTest, "Outbreak.Mission.Flow.PlayerDeathFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Phase 6 acceptance: player death fails the mission (unless the mission disables it). */
bool FOutbreakMissionFailureTest::RunTest(const FString& Parameters)
{
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		TestWorld.Spawn<AOutbreakMissionMarker>()->SetMarkerId(ExitMarker);
		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(MakeMission());
		Mission->RegisterPlayer(Player);
		Mission->StartMission();

		Player->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
		TestTrue(TEXT("Player death fails the mission"), Mission->GetMissionState() == EOutbreakMissionState::Failed);
		TestFalse(TEXT("Failure has a reason"), Mission->BuildStatus().ResultText.IsEmpty());
	}
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		UOutbreakMissionDefinition* Definition = MakeMission();
		Definition->bFailOnPlayerDeath = false;
		Mission->InitializeMission(Definition);
		Mission->RegisterPlayer(Player);
		Mission->StartMission();

		Player->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
		TestTrue(TEXT("Optional: death does not fail"), Mission->GetMissionState() == EOutbreakMissionState::InProgress);
	}
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerCharacter* PlayerA = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		AOutbreakPlayerCharacter* PlayerB = TestWorld.Spawn<AOutbreakPlayerCharacter>(FVector(300.0f, 0.0f, 0.0f));
		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(MakeMission());
		Mission->RegisterPlayer(PlayerA);
		Mission->RegisterPlayer(PlayerB);
		Mission->StartMission();

		PlayerA->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
		TestTrue(TEXT("Co-op: one player down is not a failure"), Mission->GetMissionState() == EOutbreakMissionState::InProgress);
		TestEqual(TEXT("One living player"), Mission->GetLivingPlayerPawns().Num(), 1);
		PlayerB->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
		TestTrue(TEXT("Co-op: all players dead fails"), Mission->GetMissionState() == EOutbreakMissionState::Failed);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakMissionSetupErrorsTest, "Outbreak.Mission.Flow.LevelSetupErrors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** A missing marker fails the objective visibly; an elimination with no tagged targets completes instead of soft-locking. */
bool FOutbreakMissionSetupErrorsTest::RunTest(const FString& Parameters)
{
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(MakeMission());
		Mission->RegisterPlayer(Player);

		AddExpectedError(TEXT("no AOutbreakMissionMarker with ID"), EAutomationExpectedErrorFlags::Contains, 1);
		Mission->StartMission();
		Mission->UpdateMission();
		TestTrue(TEXT("Missing marker fails the mission"), Mission->GetMissionState() == EOutbreakMissionState::Failed);
	}
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		AOutbreakMissionMarker* Marker = TestWorld.Spawn<AOutbreakMissionMarker>();
		Marker->SetMarkerId(ExitMarker);
		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(MakeMission());
		Mission->RegisterPlayer(Player);
		Mission->StartMission();

		Mission->UpdateMission();   // Player stands on the marker: objective 1 done.
		Mission->UpdateMission();   // No tagged targets: objective 2 done.
		TestTrue(TEXT("No targets: completes"), Mission->GetMissionState() == EOutbreakMissionState::Completed);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
