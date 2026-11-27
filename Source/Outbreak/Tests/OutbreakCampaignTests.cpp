#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Campaign/OutbreakCampaignDefinition.h"
#include "Campaign/OutbreakCampaignSaveGame.h"
#include "Campaign/OutbreakCampaignSettings.h"
#include "Campaign/OutbreakCampaignSubsystem.h"
#include "Campaign/OutbreakCampaignText.h"
#include "Campaign/OutbreakCampaignTypes.h"
#include "Combat/OutbreakCombatTestTarget.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Engine/World.h"
#include "Game/OutbreakGameUserSettings.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Inventory/OutbreakItemDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/OutbreakAlarmSubsystem.h"
#include "Missions/OutbreakAvoidAlarmObjective.h"
#include "Missions/OutbreakCompositeObjective.h"
#include "Missions/OutbreakEliminateTargetObjective.h"
#include "Missions/OutbreakMissionCheckpoint.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Missions/OutbreakObjectiveGraph.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Squad/OutbreakSquadSnapshot.h"
#include "Tests/OutbreakTestWorld.h"
#include "UObject/Package.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakFireBehavior.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

namespace OutbreakCampaignTests
{
	using EA = EOutbreakObjectiveActivation;
	using ES = EOutbreakObjectiveState;
	using EU = EOutbreakMissionUnlock;
	using EC = EOutbreakCampaignMissionStatus;

	UOutbreakMissionDefinition* MakeMission(FName Id)
	{
		UOutbreakMissionDefinition* Mission = NewObject<UOutbreakMissionDefinition>(GetTransientPackage());
		Mission->MissionId = Id;
		Mission->DisplayName = FText::FromName(Id);
		Mission->RestartDelay = 0.0f;
		// These missions register no player; without this the squad-down check would fail them at once.
		Mission->bFailOnPlayerDeath = false;
		return Mission;
	}

	FOutbreakCampaignMission& AddMission(UOutbreakCampaignDefinition* Campaign, FName Id, EU Unlock = EU::AfterPrevious,
		TArray<FName> Required = {}, bool bOptional = false)
	{
		FOutbreakCampaignMission& Entry = Campaign->Missions.AddDefaulted_GetRef();
		Entry.MissionId = Id;
		Entry.Mission = MakeMission(Id);
		Entry.Map = TSoftObjectPtr<UWorld>(FSoftObjectPath(FString::Printf(TEXT("/Game/Maps/L_%s.L_%s"), *Id.ToString(), *Id.ToString())));
		Entry.Unlock = Unlock;
		Entry.RequiredMissions = MoveTemp(Required);
		Entry.bOptional = bOptional;
		return Entry;
	}

	/** A (required) -> B (required) -> C (required, after A and B); Side after A; Bonus side mission at start. */
	UOutbreakCampaignDefinition* MakeCampaign()
	{
		UOutbreakCampaignDefinition* Campaign = NewObject<UOutbreakCampaignDefinition>(GetTransientPackage());
		Campaign->CampaignId = TEXT("TestCampaign");
		Campaign->DisplayName = FText::FromString(TEXT("Test Campaign"));
		Campaign->CompletionText = FText::FromString(TEXT("Well done."));
		AddMission(Campaign, TEXT("A"));
		AddMission(Campaign, TEXT("Side"), EU::AfterPrevious, {}, /*bOptional=*/ true);
		AddMission(Campaign, TEXT("B"));
		AddMission(Campaign, TEXT("C"), EU::AfterMissions, { TEXT("A"), TEXT("B") });
		AddMission(Campaign, TEXT("Bonus"), EU::AtCampaignStart, {}, /*bOptional=*/ true);
		return Campaign;
	}

	FOutbreakMissionResults MakeResults(FName Id, bool bCompleted, float Duration, int32 OptionalDone = 0, int32 OptionalCount = 0)
	{
		FOutbreakMissionResults Results;
		Results.MissionId = Id;
		Results.Outcome = bCompleted ? EOutbreakMissionState::Completed : EOutbreakMissionState::Failed;
		Results.Duration = Duration;
		Results.OptionalObjectivesCompleted = OptionalDone;
		Results.OptionalObjectiveCount = OptionalCount;
		return Results;
	}

	FOutbreakCampaignProgress MakeProgress(const UOutbreakCampaignDefinition* Campaign)
	{
		FOutbreakCampaignProgress Progress;
		Progress.CampaignId = Campaign->GetResolvedCampaignId();
		return Progress;
	}

	UOutbreakEliminateTargetObjective* AddEliminate(UOutbreakMissionDefinition* Mission, FName Id, FName Tag,
		EA Activation = EA::AfterPrevious, bool bOptional = false)
	{
		UOutbreakEliminateTargetObjective* Objective = NewObject<UOutbreakEliminateTargetObjective>(Mission);
		Objective->Description = FText::FromName(Id);
		Objective->ObjectiveId = Id;
		Objective->TargetTag = Tag;
		Objective->Activation = Activation;
		Objective->bOptional = bOptional;
		Mission->Objectives.Add(Objective);
		return Objective;
	}

	AOutbreakCombatTestTarget* SpawnTarget(FOutbreakTestWorld& TestWorld, FName Tag, float Y)
	{
		AOutbreakCombatTestTarget* Target = TestWorld.Spawn<AOutbreakCombatTestTarget>(FVector(0.0f, Y, 0.0f));
		Target->Tags.Add(Tag);
		return Target;
	}

	void Kill(AActor* Target)
	{
		Target->FindComponentByClass<UOutbreakHealthComponent>()->ApplyDamage(100000.0f, nullptr);
	}

	UOutbreakWeaponDefinition* MakeWeapon(UOutbreakAmmoTypeDefinition* Ammo, int32 MagazineCapacity)
	{
		UOutbreakWeaponDefinition* Weapon = NewObject<UOutbreakWeaponDefinition>(GetTransientPackage());
		Weapon->MagazineCapacity = MagazineCapacity;
		Weapon->ReloadDuration = 0.0f;
		for (int32 Mode = 0; Mode < 2; ++Mode)
		{
			FOutbreakFireModeDefinition& FireMode = Weapon->FireModes.AddDefaulted_GetRef();
			FireMode.FireBehavior = NewObject<UOutbreakHitscanFireBehavior>(Weapon);
		}
		Weapon->CompatibleAmmoTypes.Add(Ammo);
		return Weapon;
	}

	/** Turns the autosave off for the duration of a test, so tests never touch the player's saves. */
	struct FNoAutosave
	{
		bool bPrevious;
		FNoAutosave() : bPrevious(GetDefault<UOutbreakCampaignSettings>()->bAutosave)
		{
			GetMutableDefault<UOutbreakCampaignSettings>()->bAutosave = false;
		}
		~FNoAutosave()
		{
			GetMutableDefault<UOutbreakCampaignSettings>()->bAutosave = bPrevious;
		}
	};
}

using namespace OutbreakCampaignTests;

// --- Campaign definition -------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakCampaignDefinitionTest, "Outbreak.Campaign.Definition.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Valid campaigns pass; IDs, assets, unlock references and starting missions are checked. */
bool FOutbreakCampaignDefinitionTest::RunTest(const FString& Parameters)
{
	TArray<FText> Errors;
	TestTrue(TEXT("Test campaign is valid"), MakeCampaign()->Validate(Errors));
	TestEqual(TEXT("No errors"), Errors.Num(), 0);

	auto Rejects = [this](const TCHAR* What, UOutbreakCampaignDefinition* Campaign)
	{
		TArray<FText> InvalidErrors;
		TestFalse(What, Campaign->Validate(InvalidErrors));
		TestTrue(FString::Printf(TEXT("%s: has a reason"), What), InvalidErrors.Num() > 0);
	};

	UOutbreakCampaignDefinition* Empty = NewObject<UOutbreakCampaignDefinition>(GetTransientPackage());
	Rejects(TEXT("No missions"), Empty);

	UOutbreakCampaignDefinition* Duplicate = MakeCampaign();
	AddMission(Duplicate, TEXT("A"));
	Rejects(TEXT("Duplicate ID"), Duplicate);

	UOutbreakCampaignDefinition* Forward = NewObject<UOutbreakCampaignDefinition>(GetTransientPackage());
	AddMission(Forward, TEXT("A"), EU::AfterMissions, { TEXT("B") });
	AddMission(Forward, TEXT("B"));
	Rejects(TEXT("Requires a later mission (cycle-proof rule)"), Forward);

	UOutbreakCampaignDefinition* OnSide = NewObject<UOutbreakCampaignDefinition>(GetTransientPackage());
	AddMission(OnSide, TEXT("Side"), EU::AtCampaignStart, {}, true);
	AddMission(OnSide, TEXT("A"), EU::AfterMissions, { TEXT("Side") });
	Rejects(TEXT("Required mission depends on a side mission"), OnSide);

	UOutbreakCampaignDefinition* NoAssets = NewObject<UOutbreakCampaignDefinition>(GetTransientPackage());
	AddMission(NoAssets, TEXT("A")).Mission.Reset();
	NoAssets->Missions[0].Map.Reset();
	Rejects(TEXT("Missing mission and map"), NoAssets);

	UOutbreakCampaignDefinition* OnlySide = NewObject<UOutbreakCampaignDefinition>(GetTransientPackage());
	AddMission(OnlySide, TEXT("Side"), EU::AtCampaignStart, {}, true);
	Rejects(TEXT("Only side missions"), OnlySide);

	UOutbreakCampaignDefinition* NoStart = NewObject<UOutbreakCampaignDefinition>(GetTransientPackage());
	AddMission(NoStart, TEXT("A"), EU::AfterMissions, {});
	Rejects(TEXT("Nothing available at the start"), NoStart);
	return true;
}

// --- Progression and unlocking -------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakCampaignProgressionTest, "Outbreak.Campaign.Progression.UnlockAndComplete",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Missions unlock by their rules; failures are recorded without unlocking; required missions complete the campaign. */
bool FOutbreakCampaignProgressionTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakCampaignRules;
	UOutbreakCampaignDefinition* Campaign = MakeCampaign();
	FOutbreakCampaignProgress Progress = MakeProgress(Campaign);
	const FDateTime Now(2026, 11, 27, 12, 0, 0);

	auto Status = [&](int32 Index) { return GetMissionStatus(*Campaign, Progress, Index); };
	TestTrue(TEXT("First mission available"), Status(0) == EC::Available);
	TestTrue(TEXT("Side mission after A locked"), Status(1) == EC::Locked);
	TestTrue(TEXT("B locked (after previous required = A, skipping the side mission)"), Status(2) == EC::Locked);
	TestTrue(TEXT("C locked"), Status(3) == EC::Locked);
	TestTrue(TEXT("Bonus available at start"), Status(4) == EC::Available);
	TestEqual(TEXT("Next = A"), FindNextMission(*Campaign, Progress), FName(TEXT("A")));

	FOutbreakCampaignRecordResult Result = RecordResults(*Campaign, Progress, TEXT("A"), MakeResults(TEXT("A"), false, 50.0f), Now);
	TestTrue(TEXT("Failure recorded"), Result.bRecorded);
	TestEqual(TEXT("Failure unlocks nothing"), Result.NewlyUnlocked.Num(), 0);
	TestEqual(TEXT("Attempt counted"), Progress.FindRecord(TEXT("A"))->Attempts, 1);
	TestFalse(TEXT("Not completed"), Progress.IsMissionCompleted(TEXT("A")));

	Progress.Checkpoint.MissionId = TEXT("A");
	Result = RecordResults(*Campaign, Progress, TEXT("A"), MakeResults(TEXT("A"), true, 120.0f, 1, 2), Now);
	TestTrue(TEXT("A completed"), Progress.IsMissionCompleted(TEXT("A")));
	TestTrue(TEXT("A's checkpoint cleared"), !Progress.Checkpoint.IsSet());
	TestTrue(TEXT("Side and B unlocked"), Result.NewlyUnlocked.Contains(TEXT("Side")) && Result.NewlyUnlocked.Contains(TEXT("B")));
	TestTrue(TEXT("C still locked (needs B)"), Status(3) == EC::Locked);
	TestEqual(TEXT("Next = B (required before side missions)"), FindNextMission(*Campaign, Progress), FName(TEXT("B")));
	TestEqual(TEXT("Best time"), static_cast<double>(Progress.FindRecord(TEXT("A"))->BestDuration), 120.0, 0.001);

	// Replaying a completed mission keeps the best result.
	RecordResults(*Campaign, Progress, TEXT("A"), MakeResults(TEXT("A"), true, 200.0f, 2, 2), Now);
	const FOutbreakMissionRecord* RecordA = Progress.FindRecord(TEXT("A"));
	TestEqual(TEXT("Best time kept"), static_cast<double>(RecordA->BestDuration), 120.0, 0.001);
	TestEqual(TEXT("Best optional raised"), RecordA->BestOptionalObjectives, 2);
	TestEqual(TEXT("Attempts"), RecordA->Attempts, 3);
	TestEqual(TEXT("Completions"), RecordA->Completions, 2);
	TestTrue(TEXT("Completed mission stays completed"), Status(0) == EC::Completed);

	TestFalse(TEXT("Unknown mission not recorded"), RecordResults(*Campaign, Progress, TEXT("Nope"), MakeResults(TEXT("Nope"), true, 1.0f), Now).bRecorded);

	RecordResults(*Campaign, Progress, TEXT("B"), MakeResults(TEXT("B"), true, 100.0f), Now);
	TestTrue(TEXT("C unlocked after A and B"), Status(3) == EC::Available);
	TestFalse(TEXT("Not complete yet"), IsCampaignComplete(*Campaign, Progress));

	Result = RecordResults(*Campaign, Progress, TEXT("C"), MakeResults(TEXT("C"), true, 100.0f), Now);
	TestTrue(TEXT("Campaign completed by the last required mission"), Result.bCampaignCompleted);
	TestTrue(TEXT("Completion time set"), Progress.CompletedAt == Now);
	TestTrue(TEXT("Complete without side missions"), IsCampaignComplete(*Campaign, Progress));
	TestEqual(TEXT("Next = a side mission"), FindNextMission(*Campaign, Progress), FName(TEXT("Side")));

	Result = RecordResults(*Campaign, Progress, TEXT("C"), MakeResults(TEXT("C"), true, 90.0f), Now + FTimespan::FromDays(1));
	TestFalse(TEXT("Completion reported once"), Result.bCampaignCompleted);
	TestTrue(TEXT("Completion time kept"), Progress.CompletedAt == Now);

	int32 Completed = 0;
	int32 Total = 0;
	CountRequired(*Campaign, Progress, Completed, Total);
	TestTrue(TEXT("3 / 3 required"), Completed == 3 && Total == 3);
	TestEqual(TEXT("Total mission time"), static_cast<double>(Progress.TotalMissionTime), 50.0 + 120.0 + 200.0 + 100.0 + 100.0 + 90.0, 0.001);
	return true;
}

// --- Save validation and round trip --------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSaveValidationTest, "Outbreak.Campaign.Save.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Saving refuses inconsistent progress; loading rejects unusable saves and repairs fixable ones. */
bool FOutbreakSaveValidationTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakSaveValidation;
	UOutbreakCampaignDefinition* Campaign = MakeCampaign();
	const FDateTime Now(2026, 11, 27, 12, 0, 0);

	FOutbreakCampaignProgress Good = MakeProgress(Campaign);
	OutbreakCampaignRules::RecordResults(*Campaign, Good, TEXT("A"), MakeResults(TEXT("A"), true, 60.0f), Now);
	Good.Checkpoint.MissionId = TEXT("B");
	Good.Checkpoint.ObjectiveStates = { ES::Completed, ES::Inactive };
	Good.Checkpoint.Squad.SquadSize = 2;
	Good.Checkpoint.Squad.TeammateRosterIndices = { INDEX_NONE };
	Good.Checkpoint.Squad.Members.SetNum(2);

	TArray<FText> Errors;
	TestTrue(TEXT("Consistent progress can be saved"), ValidateForSave(Good, *Campaign, Errors));

	auto RefusesSave = [&](const TCHAR* What, TFunctionRef<void(FOutbreakCampaignProgress&)> Break)
	{
		FOutbreakCampaignProgress Bad = Good;
		Break(Bad);
		TArray<FText> BadErrors;
		TestFalse(What, ValidateForSave(Bad, *Campaign, BadErrors));
	};
	RefusesSave(TEXT("Other campaign"), [](FOutbreakCampaignProgress& P) { P.CampaignId = TEXT("Other"); });
	RefusesSave(TEXT("Unknown mission record"), [](FOutbreakCampaignProgress& P) { P.FindOrAddRecord(TEXT("Ghost")); });
	RefusesSave(TEXT("Duplicate record"), [](FOutbreakCampaignProgress& P) { P.Missions.Add(P.Missions[0]); });
	RefusesSave(TEXT("Inconsistent counts"), [](FOutbreakCampaignProgress& P) { P.Missions[0].Completions = 5; });
	RefusesSave(TEXT("Checkpoint in a locked mission"), [](FOutbreakCampaignProgress& P) { P.Checkpoint.MissionId = TEXT("C"); });
	RefusesSave(TEXT("Checkpoint squad above the limit"), [](FOutbreakCampaignProgress& P)
	{
		P.Checkpoint.Squad.SquadSize = 99;
	});

	// Loading.
	TArray<FText> Warnings;
	Errors.Reset();
	TestFalse(TEXT("Missing save rejected"), ValidateLoaded(nullptr, *Campaign, Errors, Warnings));
	TestFalse(TEXT("Foreign save class rejected"), ValidateLoaded(NewObject<USaveGame>(), *Campaign, Errors, Warnings));

	auto MakeSave = [&Good]()
	{
		UOutbreakCampaignSaveGame* Save = NewObject<UOutbreakCampaignSaveGame>();
		Save->Progress = Good;
		return Save;
	};
	UOutbreakCampaignSaveGame* Newer = MakeSave();
	Newer->SaveVersion = OutbreakSaveVersion::Current + 1;
	TestFalse(TEXT("Newer version rejected"), ValidateLoaded(Newer, *Campaign, Errors, Warnings));
	UOutbreakCampaignSaveGame* Invalid = MakeSave();
	Invalid->SaveVersion = 0;
	TestFalse(TEXT("Invalid version rejected"), ValidateLoaded(Invalid, *Campaign, Errors, Warnings));
	UOutbreakCampaignSaveGame* Other = MakeSave();
	Other->Progress.CampaignId = TEXT("Other");
	TestFalse(TEXT("Other campaign rejected"), ValidateLoaded(Other, *Campaign, Errors, Warnings));

	Errors.Reset();
	Warnings.Reset();
	TestTrue(TEXT("Good save loads"), ValidateLoaded(MakeSave(), *Campaign, Errors, Warnings));
	TestEqual(TEXT("Good save: no warnings"), Warnings.Num(), 0);

	UOutbreakCampaignSaveGame* Damaged = MakeSave();
	Damaged->Progress.FindOrAddRecord(TEXT("Ghost")).Attempts = 3;
	Damaged->Progress.Missions[0].Completions = 7;
	Damaged->Progress.Missions[0].Attempts = 2;
	Damaged->Progress.Missions.Add(Damaged->Progress.Missions[0]);
	Damaged->Progress.TotalMissionTime = -5.0f;
	Damaged->Progress.Checkpoint.ObjectiveStates.Reset();
	Damaged->Progress.CompletedAt = Now;
	Errors.Reset();
	Warnings.Reset();
	TestTrue(TEXT("Damaged save repaired"), ValidateLoaded(Damaged, *Campaign, Errors, Warnings));
	const FOutbreakCampaignProgress& Repaired = Damaged->Progress;
	TestNull(TEXT("Unknown record dropped"), Repaired.FindRecord(TEXT("Ghost")));
	TestEqual(TEXT("Duplicates merged"), Repaired.Missions.Num(), 1);
	TestTrue(TEXT("Counts consistent"), Repaired.Missions[0].Completions <= Repaired.Missions[0].Attempts);
	TestEqual(TEXT("Total time reset"), static_cast<double>(Repaired.TotalMissionTime), 0.0, 0.001);
	TestFalse(TEXT("Unusable checkpoint discarded"), Repaired.Checkpoint.IsSet());
	TestTrue(TEXT("Completion not trusted from the file"), Repaired.CompletedAt == FDateTime());
	TestTrue(TEXT("Repairs reported"), Warnings.Num() >= 4);
	TestTrue(TEXT("Repaired progress can be saved again"), ValidateForSave(Repaired, *Campaign, Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSaveRoundTripTest, "Outbreak.Campaign.Save.RoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Progress, records and a checkpoint with soft asset references survive serialization (in memory, no disk). */
bool FOutbreakSaveRoundTripTest::RunTest(const FString& Parameters)
{
	UOutbreakCampaignDefinition* Campaign = MakeCampaign();
	UOutbreakAmmoTypeDefinition* Ammo = NewObject<UOutbreakAmmoTypeDefinition>(GetTransientPackage());
	UOutbreakWeaponDefinition* Weapon = MakeWeapon(Ammo, 12);

	UOutbreakCampaignSaveGame* Save = NewObject<UOutbreakCampaignSaveGame>();
	Save->Progress = MakeProgress(Campaign);
	OutbreakCampaignRules::RecordResults(*Campaign, Save->Progress, TEXT("A"), MakeResults(TEXT("A"), true, 75.0f, 1, 3), FDateTime(2026, 11, 27));
	FOutbreakMissionCheckpoint& Checkpoint = Save->Progress.Checkpoint;
	Checkpoint.MissionId = TEXT("B");
	Checkpoint.ObjectiveStates = { ES::Completed, ES::Failed, ES::Inactive };
	Checkpoint.MissionTime = 321.5f;
	Checkpoint.ResumeMarkerId = TEXT("Gate");
	Checkpoint.bAlarmRaised = true;
	Checkpoint.Squad.SquadSize = 2;
	Checkpoint.Squad.TeammateRosterIndices = { 1 };
	FOutbreakSoldierSnapshot& Soldier = Checkpoint.Squad.Members.AddDefaulted_GetRef();
	Soldier.HealthFraction = 0.4f;
	FOutbreakSavedWeapon& SavedWeapon = Soldier.Weapons.AddDefaulted_GetRef();
	SavedWeapon.Weapon = Weapon;
	SavedWeapon.AmmoInMagazine = 7;
	Save->SavedAt = FDateTime(2026, 11, 27, 9, 30, 0);

	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Serialized"), UGameplayStatics::SaveGameToMemory(Save, Bytes)))
	{
		return false;
	}
	UOutbreakCampaignSaveGame* Loaded = Cast<UOutbreakCampaignSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("Deserialized as a campaign save"), Loaded))
	{
		return false;
	}

	TArray<FText> Errors;
	TArray<FText> Warnings;
	TestTrue(TEXT("Loaded save validates"), OutbreakSaveValidation::ValidateLoaded(Loaded, *Campaign, Errors, Warnings));
	const FOutbreakCampaignProgress& Progress = Loaded->Progress;
	TestEqual(TEXT("Campaign ID"), Progress.CampaignId, FName(TEXT("TestCampaign")));
	TestTrue(TEXT("Record kept"), Progress.IsMissionCompleted(TEXT("A")));
	TestEqual(TEXT("Best optional"), Progress.FindRecord(TEXT("A"))->BestOptionalObjectives, 1);
	TestEqual(TEXT("Checkpoint mission"), Progress.Checkpoint.MissionId, FName(TEXT("B")));
	TestTrue(TEXT("Objective states"), Progress.Checkpoint.ObjectiveStates == TArray<ES>({ ES::Completed, ES::Failed, ES::Inactive }));
	TestEqual(TEXT("Mission time"), static_cast<double>(Progress.Checkpoint.MissionTime), 321.5, 0.001);
	TestEqual(TEXT("Resume marker"), Progress.Checkpoint.ResumeMarkerId, FName(TEXT("Gate")));
	TestTrue(TEXT("Alarm"), Progress.Checkpoint.bAlarmRaised);
	TestEqual(TEXT("Soldier health"), static_cast<double>(Progress.Checkpoint.Squad.Members[0].HealthFraction), 0.4, 0.001);
	TestTrue(TEXT("Weapon reference resolves"), Progress.Checkpoint.Squad.Members[0].Weapons[0].Weapon.Get() == Weapon);
	TestTrue(TEXT("Saved time"), Loaded->SavedAt == FDateTime(2026, 11, 27, 9, 30, 0));
	return true;
}

// --- Campaign subsystem --------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakCampaignSubsystemTest, "Outbreak.Campaign.Subsystem.LaunchAndRecord",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Launch rules, map matching, checkpoints and results through the subsystem (no travel, no disk). */
bool FOutbreakCampaignSubsystemTest::RunTest(const FString& Parameters)
{
	FNoAutosave NoAutosave;
	UOutbreakCampaignSubsystem* Subsystem = NewObject<UOutbreakCampaignSubsystem>(GetTransientPackage());
	UOutbreakCampaignDefinition* Campaign = MakeCampaign();
	if (!TestTrue(TEXT("Campaign set"), Subsystem->SetCampaign(Campaign)))
	{
		return false;
	}

	AddExpectedError(TEXT("no campaign is active"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("No launch before a campaign is active"), Subsystem->PrepareLaunch(TEXT("A"), false));

	TestTrue(TEXT("New campaign"), Subsystem->StartNewCampaign());
	AddExpectedError(TEXT("unknown or locked"), EAutomationExpectedErrorFlags::Contains, 2);
	TestFalse(TEXT("Locked mission refused"), Subsystem->PrepareLaunch(TEXT("B"), false));
	TestFalse(TEXT("Unknown mission refused"), Subsystem->PrepareLaunch(TEXT("Nope"), false));

	TestTrue(TEXT("A launches"), Subsystem->PrepareLaunch(TEXT("A"), false));
	TestNotNull(TEXT("Launched on its map"), Subsystem->GetLaunchedMission(TEXT("L_A")));
	TestNotNull(TEXT("PIE prefix ignored"), Subsystem->GetLaunchedMission(TEXT("UEDPIE_0_L_A")));
	TestNotNull(TEXT("Long name accepted"), Subsystem->GetLaunchedMission(TEXT("/Game/Maps/L_A")));
	TestNull(TEXT("Other maps run their own mission"), Subsystem->GetLaunchedMission(TEXT("L_B")));
	TestNull(TEXT("Fresh launch: no checkpoint"), Subsystem->GetCheckpointToResume());

	FOutbreakMissionCheckpoint Checkpoint;
	Checkpoint.MissionId = TEXT("DefinitionId");
	Checkpoint.ObjectiveStates = { ES::Completed, ES::Inactive };
	Checkpoint.MissionTime = 42.0f;
	Subsystem->RecordCheckpoint(Checkpoint);
	TestTrue(TEXT("Checkpoint kept"), Subsystem->HasCheckpoint());
	TestEqual(TEXT("Checkpoint uses the campaign's mission ID"), Subsystem->GetProgress().Checkpoint.MissionId, FName(TEXT("A")));

	FOutbreakCampaignRecordResult Recorded = Subsystem->RecordMissionResults(MakeResults(TEXT("A"), false, 80.0f));
	FOutbreakCampaignStatus Status = Subsystem->BuildStatus(&Recorded);
	TestTrue(TEXT("Campaign mission"), Status.bCampaignMission);
	TestTrue(TEXT("Failure keeps the checkpoint"), Status.bHasCheckpoint);
	TestFalse(TEXT("No next mission after a failure"), Status.bHasNextMission);
	TestTrue(TEXT("Summary offers the checkpoint"), OutbreakCampaignText::GetSummaryOptions(Status, MakeResults(TEXT("A"), false, 0.0f)).bCheckpoint);

	Subsystem->SetResumeOnRestart(true);
	const FOutbreakMissionCheckpoint* Resume = Subsystem->GetCheckpointToResume();
	TestTrue(TEXT("Retry from checkpoint resumes it"), Resume && Resume->MissionTime == 42.0f);
	Subsystem->SetResumeOnRestart(false);
	TestFalse(TEXT("Restart from the briefing abandons it"), Subsystem->HasCheckpoint());

	Subsystem->RecordCheckpoint(Checkpoint);
	Recorded = Subsystem->RecordMissionResults(MakeResults(TEXT("A"), true, 90.0f));
	Status = Subsystem->BuildStatus(&Recorded);
	TestFalse(TEXT("Completion clears the checkpoint"), Status.bHasCheckpoint);
	TestTrue(TEXT("Next mission offered"), Status.bHasNextMission);
	TestEqual(TEXT("Next is B"), Status.NextMissionName.ToString(), FString(TEXT("B")));
	TestTrue(TEXT("Unlocked missions listed"), Status.UnlockedMissionNames.Num() == 2);
	TestTrue(TEXT("Summary offers Continue"), OutbreakCampaignText::GetSummaryOptions(Status, MakeResults(TEXT("A"), true, 0.0f)).bContinue);

	// A save of this progress applies to a fresh subsystem.
	UOutbreakCampaignSaveGame* Save = Subsystem->CreateSaveObject();
	UOutbreakCampaignSubsystem* Other = NewObject<UOutbreakCampaignSubsystem>(GetTransientPackage());
	Other->SetCampaign(Campaign);
	TArray<FText> Errors;
	TestTrue(TEXT("Save applies"), Other->ApplyLoadedSave(Save, Errors));
	TestTrue(TEXT("Loaded campaign active"), Other->IsCampaignActive());
	TestTrue(TEXT("Loaded progress"), Other->GetMissionStatus(TEXT("A")) == EC::Completed && Other->GetNextMissionId() == FName(TEXT("B")));
	return true;
}

// --- Checkpoints ---------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakCheckpointRulesTest, "Outbreak.Campaign.Checkpoint.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** A checkpoint must match the mission's objectives and describe a mission still in progress. */
bool FOutbreakCheckpointRulesTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakMissionCheckpointRules;
	auto Node = [](FName Id, EA Activation, bool bOptional = false)
	{
		FOutbreakObjectiveNode Result;
		Result.Id = Id;
		Result.Activation = Activation;
		Result.bOptional = bOptional;
		return Result;
	};
	const TArray<FOutbreakObjectiveNode> Nodes = { Node(TEXT("A"), EA::AfterPrevious), Node(TEXT("Opt"), EA::AtMissionStart, true), Node(TEXT("B"), EA::AfterPrevious) };

	FOutbreakMissionCheckpoint Checkpoint;
	Checkpoint.MissionId = TEXT("M");
	auto Valid = [&](std::initializer_list<ES> States)
	{
		Checkpoint.ObjectiveStates = States;
		TArray<FText> Errors;
		return Validate(Checkpoint, Nodes, Errors);
	};
	TestTrue(TEXT("A done, optional failed, B to do"), Valid({ ES::Completed, ES::Failed, ES::Inactive }));
	TestTrue(TEXT("Active saved states are fine (they restart)"), Valid({ ES::Completed, ES::Active, ES::Active }));
	TestFalse(TEXT("Wrong objective count"), Valid({ ES::Completed, ES::Inactive }));
	TestFalse(TEXT("Required objective failed"), Valid({ ES::Failed, ES::Inactive, ES::Inactive }));
	TestFalse(TEXT("Completed out of order"), Valid({ ES::Inactive, ES::Inactive, ES::Completed }));
	TestFalse(TEXT("Nothing left to do"), Valid({ ES::Completed, ES::Inactive, ES::Completed }));
	TestTrue(TEXT("Active becomes Inactive"), ToSavedState(ES::Active) == ES::Inactive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakCheckpointResumeTest, "Outbreak.Campaign.Checkpoint.CaptureAndResume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** A checkpoint taken after the first objective resumes in a fresh world with that objective done. */
bool FOutbreakCheckpointResumeTest::RunTest(const FString& Parameters)
{
	UOutbreakMissionDefinition* Definition = MakeMission(TEXT("Resume"));
	Definition->DeploymentMarkerId = TEXT("Start");
	AddEliminate(Definition, TEXT("First"), TEXT("T1"))->CheckpointMarkerId = TEXT("Gate");
	AddEliminate(Definition, TEXT("Second"), TEXT("T2"));

	FOutbreakMissionCheckpoint Checkpoint;
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakCombatTestTarget* First = SpawnTarget(TestWorld, TEXT("T1"), 1000.0f);
		SpawnTarget(TestWorld, TEXT("T2"), 2000.0f);
		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(Definition);
		Mission->StartMission();
		Mission->AdvanceMission(10.0f);
		Kill(First);
		Mission->AdvanceMission(5.0f);
		if (!TestTrue(TEXT("Second objective active"), Mission->GetObjective(1)->GetState() == ES::Active))
		{
			return false;
		}
		Checkpoint = Mission->CaptureCheckpoint(0);
	}
	TestEqual(TEXT("Checkpoint mission"), Checkpoint.MissionId, FName(TEXT("Resume")));
	TestTrue(TEXT("States saved (active as inactive)"), Checkpoint.ObjectiveStates == TArray<ES>({ ES::Completed, ES::Inactive }));
	TestEqual(TEXT("Mission time saved"), static_cast<double>(Checkpoint.MissionTime), 15.0, 0.001);
	TestEqual(TEXT("Objective's checkpoint marker"), Checkpoint.ResumeMarkerId, FName(TEXT("Gate")));
	TestEqual(TEXT("Completed count"), Checkpoint.CompletedObjectiveCount, 1);

	{
		// The level resets: the first target is alive again, but its objective stays done.
		FOutbreakTestWorld TestWorld;
		SpawnTarget(TestWorld, TEXT("T1"), 1000.0f);
		AOutbreakCombatTestTarget* Second = SpawnTarget(TestWorld, TEXT("T2"), 2000.0f);
		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(Definition);
		TestTrue(TEXT("Resumed"), Mission->StartMissionFromCheckpoint(Checkpoint));
		TestTrue(TEXT("First stays completed"), Mission->GetObjective(0)->GetState() == ES::Completed);
		TestTrue(TEXT("Second active"), Mission->GetObjective(1)->GetState() == ES::Active);
		TestEqual(TEXT("Mission time continues"), static_cast<double>(Mission->GetMissionTime()), 15.0, 0.001);

		Kill(Second);
		Mission->UpdateMission();
		TestTrue(TEXT("Mission completes"), Mission->GetMissionState() == EOutbreakMissionState::Completed);
		TestEqual(TEXT("Both required objectives in the results"), Mission->GetMissionResults().RequiredObjectivesCompleted, 2);
	}
	{
		FOutbreakTestWorld TestWorld;
		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(Definition);
		FOutbreakMissionCheckpoint Stale = Checkpoint;
		Stale.ObjectiveStates.Add(ES::Completed);
		AddExpectedError(TEXT("checkpoint not used"), EAutomationExpectedErrorFlags::Contains, 1);
		TestFalse(TEXT("A checkpoint that does not fit is refused"), Mission->StartMissionFromCheckpoint(Stale));
		TestTrue(TEXT("...and the mission starts fresh"), Mission->GetMissionState() == EOutbreakMissionState::InProgress
			&& Mission->GetObjective(0)->GetState() == ES::Active && Mission->GetMissionTime() == 0.0f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSoldierSnapshotTest, "Outbreak.Campaign.Checkpoint.SoldierSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Health, weapons (magazine, fire mode, active weapon), reserves and items are captured and restored. */
bool FOutbreakSoldierSnapshotTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Source = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakPlayerCharacter* Target = TestWorld.Spawn<AOutbreakPlayerCharacter>(FVector(500.0f, 0.0f, 0.0f));
	UOutbreakAmmoTypeDefinition* Ammo = NewObject<UOutbreakAmmoTypeDefinition>(GetTransientPackage());
	UOutbreakWeaponDefinition* Rifle = MakeWeapon(Ammo, 30);
	UOutbreakWeaponDefinition* Pistol = MakeWeapon(Ammo, 12);
	UOutbreakItemDefinition* Medkit = NewObject<UOutbreakItemDefinition>(GetTransientPackage());
	Medkit->Weight = 0.5f;
	Medkit->MaxStack = 5;

	UOutbreakWeaponComponent* SourceWeapons = Source->FindComponentByClass<UOutbreakWeaponComponent>();
	SourceWeapons->ClearLoadout();
	SourceWeapons->AddWeapon(Rifle);
	SourceWeapons->AddWeapon(Pistol);
	SourceWeapons->AddAmmo(Ammo, 40);
	SourceWeapons->EquipWeapon(1);
	SourceWeapons->CycleFireMode();
	Source->FindComponentByClass<UOutbreakInventoryComponent>()->AddItem(Medkit, 2);
	Source->FindComponentByClass<UOutbreakHealthComponent>()->ApplyDamage(35.0f, nullptr);

	const FOutbreakSoldierSnapshot Snapshot = OutbreakSquadSnapshot::CaptureSoldier(*Source);
	TestEqual(TEXT("Two weapons captured"), Snapshot.Weapons.Num(), 2);
	TestEqual(TEXT("Active weapon captured"), Snapshot.ActiveWeaponIndex, 1);

	OutbreakSquadSnapshot::ApplySoldier(*Target, Snapshot);
	const UOutbreakWeaponComponent* TargetWeapons = Target->FindComponentByClass<UOutbreakWeaponComponent>();
	const UOutbreakHealthComponent* TargetHealth = Target->FindComponentByClass<UOutbreakHealthComponent>();
	TestEqual(TEXT("Weapons restored"), TargetWeapons->GetWeaponCount(), 2);
	TestTrue(TEXT("Pistol equipped"), TargetWeapons->GetActiveWeaponDefinition() == Pistol);
	TestEqual(TEXT("Fire mode restored"), TargetWeapons->GetWeaponSlots()[1].FireModeIndex, 1);
	TestEqual(TEXT("Magazine restored"), TargetWeapons->GetAmmoInMagazine(), 12);
	TestEqual(TEXT("Reserve restored"), TargetWeapons->GetReserveAmmo(Ammo), 40);
	TestEqual(TEXT("Items restored"), Target->FindComponentByClass<UOutbreakInventoryComponent>()->GetItemCount(Medkit), 2);
	TestEqual(TEXT("Health restored"), static_cast<double>(TargetHealth->GetHealthFraction()),
		static_cast<double>(Source->FindComponentByClass<UOutbreakHealthComponent>()->GetHealthFraction()), 0.001);

	FOutbreakSoldierSnapshot Downed = Snapshot;
	Downed.HealthFraction = 0.0f;
	OutbreakSquadSnapshot::ApplySoldier(*Target, Downed);
	TestTrue(TEXT("A soldier saved while down stands up"), TargetHealth->IsAlive() && TargetHealth->GetHealth() > 0.0f);

	FOutbreakSoldierSnapshot Dead = Snapshot;
	Dead.bDead = true;
	OutbreakSquadSnapshot::ApplySoldier(*Target, Dead);
	TestTrue(TEXT("A dead soldier stays dead"), TargetHealth->GetHealthState() == EOutbreakHealthState::Dead);
	return true;
}

// --- Complex objectives --------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakComplexObjectivesTest, "Outbreak.Mission.Objectives.TimeLimitCompositeAvoidAlarm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Time limits fail objectives; composites complete with N of their parts; Avoid Alarm fails on the alarm. */
bool FOutbreakComplexObjectivesTest::RunTest(const FString& Parameters)
{
	{
		FOutbreakTestWorld TestWorld;
		SpawnTarget(TestWorld, TEXT("T"), 1000.0f);
		UOutbreakMissionDefinition* Definition = MakeMission(TEXT("Timed"));
		AddEliminate(Definition, TEXT("Timed"), TEXT("T"))->TimeLimit = 30.0f;
		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(Definition);
		Mission->StartMission();
		Mission->AdvanceMission(10.0f);
		TestEqual(TEXT("Time left shown"), Mission->BuildStatus().ObjectiveProgress.ToString().Contains(TEXT("0:20")), true);
		Mission->AdvanceMission(25.0f);
		TestTrue(TEXT("Out of time: mission failed"), Mission->GetMissionState() == EOutbreakMissionState::Failed);
		TestTrue(TEXT("Reason names the time"), Mission->GetMissionResults().ResultText.ToString().Contains(TEXT("Out of time")));
	}
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakCombatTestTarget* Generator = SpawnTarget(TestWorld, TEXT("Gen"), 1000.0f);
		SpawnTarget(TestWorld, TEXT("Relay"), 2000.0f);
		UOutbreakMissionDefinition* Definition = MakeMission(TEXT("Composite"));
		UOutbreakCompositeObjective* Either = NewObject<UOutbreakCompositeObjective>(Definition);
		Either->Description = FText::FromString(TEXT("Cut the power"));
		Either->RequiredCount = 1;
		for (const FName Tag : { FName(TEXT("Gen")), FName(TEXT("Relay")) })
		{
			UOutbreakEliminateTargetObjective* Sub = NewObject<UOutbreakEliminateTargetObjective>(Either);
			Sub->TargetTag = Tag;
			Either->SubObjectives.Add(Sub);
		}
		Definition->Objectives.Add(Either);
		TArray<FText> Errors;
		TestTrue(TEXT("Composite validates"), Definition->Validate(Errors));

		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(Definition);
		Mission->StartMission();
		Mission->UpdateMission();
		TestEqual(TEXT("Progress 0 / 1"), Mission->BuildStatus().ObjectiveProgress.ToString(), FString(TEXT("0 / 1")));
		Kill(Generator);
		Mission->UpdateMission();
		TestTrue(TEXT("One route is enough"), Mission->GetMissionState() == EOutbreakMissionState::Completed);

		Either->RequiredCount = 3;
		Errors.Reset();
		TestFalse(TEXT("Needing more parts than exist is invalid"), Definition->Validate(Errors));
	}
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakCombatTestTarget* Target = SpawnTarget(TestWorld, TEXT("T"), 1000.0f);
		UOutbreakMissionDefinition* Definition = MakeMission(TEXT("Stealth"));
		AddEliminate(Definition, TEXT("Main"), TEXT("T"));
		UOutbreakAvoidAlarmObjective* Quiet = NewObject<UOutbreakAvoidAlarmObjective>(Definition);
		Quiet->Description = FText::FromString(TEXT("Stay undetected"));
		Quiet->Activation = EA::AtMissionStart;
		Quiet->bOptional = true;
		Quiet->UntilObjectiveId = TEXT("Main");
		Definition->Objectives.Add(Quiet);

		TArray<FText> Errors;
		TestTrue(TEXT("Avoid Alarm validates"), Definition->Validate(Errors));
		Quiet->UntilObjectiveId = TEXT("Missing");
		Errors.Reset();
		TestFalse(TEXT("Unknown objective reference rejected"), Definition->Validate(Errors));
		Quiet->UntilObjectiveId = TEXT("Main");

		UOutbreakMissionSubsystem* Mission = TestWorld.Get()->GetSubsystem<UOutbreakMissionSubsystem>();
		Mission->InitializeMission(Definition);
		Mission->StartMission();
		Mission->UpdateMission();
		TestTrue(TEXT("Quiet so far"), Mission->GetObjective(1)->GetState() == ES::Active);
		TestWorld.Get()->GetSubsystem<UOutbreakAlarmSubsystem>()->RaiseAlarm(FVector::ZeroVector, nullptr);
		Mission->UpdateMission();
		TestTrue(TEXT("Alarm fails the stealth objective"), Mission->GetObjective(1)->GetState() == ES::Failed);
		TestTrue(TEXT("Optional: mission goes on"), Mission->GetMissionState() == EOutbreakMissionState::InProgress);
		Kill(Target);
		Mission->UpdateMission();
		TestTrue(TEXT("Mission completes"), Mission->GetMissionState() == EOutbreakMissionState::Completed);
	}
	return true;
}

// --- Settings and text ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakUserSettingsTest, "Outbreak.Campaign.Settings.Clamping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Settings are clamped when set, so a damaged file or a slider cannot produce unusable controls. */
bool FOutbreakUserSettingsTest::RunTest(const FString& Parameters)
{
	using Settings = UOutbreakGameUserSettings;
	TestEqual(TEXT("Sensitivity floor"), static_cast<double>(Settings::ClampLookSensitivity(0.0f)), static_cast<double>(Settings::MinLookSensitivity), 0.0001);
	TestEqual(TEXT("Sensitivity ceiling"), static_cast<double>(Settings::ClampLookSensitivity(50.0f)), static_cast<double>(Settings::MaxLookSensitivity), 0.0001);
	TestEqual(TEXT("Sensitivity NaN"), static_cast<double>(Settings::ClampLookSensitivity(std::numeric_limits<float>::quiet_NaN())), 1.0, 0.0001);
	TestEqual(TEXT("FOV 0 = character default"), static_cast<double>(Settings::ClampFieldOfView(0.0f)), 0.0, 0.0001);
	TestEqual(TEXT("FOV negative = default"), static_cast<double>(Settings::ClampFieldOfView(-10.0f)), 0.0, 0.0001);
	TestEqual(TEXT("FOV floor"), static_cast<double>(Settings::ClampFieldOfView(20.0f)), static_cast<double>(Settings::MinFieldOfView), 0.0001);
	TestEqual(TEXT("FOV ceiling"), static_cast<double>(Settings::ClampFieldOfView(170.0f)), static_cast<double>(Settings::MaxFieldOfView), 0.0001);

	UOutbreakGameUserSettings* Object = NewObject<UOutbreakGameUserSettings>(GetTransientPackage());
	Object->SetLookSensitivity(99.0f);
	Object->SetFieldOfView(100.0f);
	TestEqual(TEXT("Setter clamps sensitivity"), static_cast<double>(Object->GetLookSensitivity()), static_cast<double>(Settings::MaxLookSensitivity), 0.0001);
	TestEqual(TEXT("Setter keeps a valid FOV"), static_cast<double>(Object->GetFieldOfView()), 100.0, 0.0001);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakCampaignTextTest, "Outbreak.Campaign.Text.MenuAndSummary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Mission rows, slot rows and summary text. */
bool FOutbreakCampaignTextTest::RunTest(const FString& Parameters)
{
	using namespace OutbreakCampaignText;
	FOutbreakMissionRecord Record;
	Record.bCompleted = true;
	Record.BestDuration = 754.0f;
	Record.BestOptionalObjectives = 1;
	Record.OptionalObjectiveCount = 2;
	const FString Completed = BuildMissionLine(1, FText::FromString(TEXT("Relay")), EC::Completed, &Record, false, {}, false).ToString();
	TestTrue(TEXT("Completed row"), Completed.StartsWith(TEXT("2. Relay")) && Completed.Contains(TEXT("Completed")) && Completed.Contains(TEXT("12:34"))
		&& Completed.Contains(TEXT("1 / 2")));
	const FString Locked = BuildMissionLine(2, FText::FromString(TEXT("Core")), EC::Locked, nullptr, false, { FText::FromString(TEXT("Relay")) }, false).ToString();
	TestTrue(TEXT("Locked row names what unlocks it"), Locked.Contains(TEXT("complete Relay first")));
	TestTrue(TEXT("Checkpoint shown"), BuildMissionLine(0, FText::FromString(TEXT("A")), EC::Available, nullptr, true, {}, false).ToString().Contains(TEXT("checkpoint")));
	TestTrue(TEXT("Side mission marked"), BuildMissionLine(0, FText::FromString(TEXT("A")), EC::Available, nullptr, false, {}, true).ToString().Contains(TEXT("side mission")));

	FOutbreakSaveSlotSummary Slot;
	Slot.SlotIndex = 2;
	TestEqual(TEXT("Empty slot"), BuildSlotLine(Slot).ToString(), FString(TEXT("Slot 2 - empty")));
	Slot.bExists = true;
	Slot.bUnreadable = true;
	TestTrue(TEXT("Unreadable slot"), BuildSlotLine(Slot).ToString().Contains(TEXT("unreadable")));
	Slot.bUnreadable = false;
	Slot.SlotIndex = 0;
	Slot.CurrentMissionName = FText::FromString(TEXT("Relay"));
	Slot.CompletedMissions = 1;
	Slot.RequiredMissions = 3;
	Slot.bHasCheckpoint = true;
	const FString Auto = BuildSlotLine(Slot).ToString();
	TestTrue(TEXT("Autosave row"), Auto.StartsWith(TEXT("Autosave")) && Auto.Contains(TEXT("Relay")) && Auto.Contains(TEXT("1 / 3")) && Auto.Contains(TEXT("checkpoint")));

	FOutbreakCampaignStatus Status;
	TestTrue(TEXT("Stand-alone mission: no campaign text"), BuildSummaryCampaignText(Status, MakeResults(TEXT("A"), true, 1.0f)).IsEmpty());
	Status.bCampaignMission = true;
	Status.bCampaignJustCompleted = true;
	Status.CampaignName = FText::FromString(TEXT("Outbreak"));
	Status.UnlockedMissionNames = { FText::FromString(TEXT("Epilogue")) };
	const FString Summary = BuildSummaryCampaignText(Status, MakeResults(TEXT("A"), true, 1.0f)).ToString();
	TestTrue(TEXT("Completion announced"), Summary.Contains(TEXT("CAMPAIGN COMPLETE")));
	TestTrue(TEXT("Unlocks listed"), Summary.Contains(TEXT("Unlocked: Epilogue")));
	TestTrue(TEXT("Progress line"), BuildProgressLine(1, 3, 600.0f).ToString().Contains(TEXT("1 / 3")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
