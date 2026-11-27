#include "Campaign/OutbreakCampaignSubsystem.h"

#include "Campaign/OutbreakCampaignDefinition.h"
#include "Campaign/OutbreakCampaignSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "Outbreak.h"

#define LOCTEXT_NAMESPACE "OutbreakCampaign"

namespace
{
	/** The local player's save user. Single-player saves; co-op clients do not save. */
	constexpr int32 SaveUserIndex = 0;

	void LogMessages(const TCHAR* Context, const TArray<FText>& Messages, ELogVerbosity::Type Verbosity)
	{
		for (const FText& Message : Messages)
		{
			if (Verbosity == ELogVerbosity::Error)
			{
				UE_LOG(LogOutbreak, Error, TEXT("%s: %s"), Context, *Message.ToString());
			}
			else
			{
				UE_LOG(LogOutbreak, Warning, TEXT("%s: %s"), Context, *Message.ToString());
			}
		}
	}
}

// --- Campaign ------------------------------------------------------------------------------------

UOutbreakCampaignDefinition* UOutbreakCampaignSubsystem::GetCampaign() const
{
	if (!Campaign)
	{
		// Loaded on first use and cached; the cache is the only state a const query changes.
		const UOutbreakCampaignSettings* Settings = GetDefault<UOutbreakCampaignSettings>();
		UOutbreakCampaignDefinition* Loaded = Settings->Campaign.IsNull() ? nullptr : Settings->Campaign.LoadSynchronous();
		TArray<FText> Errors;
		if (Loaded && !Loaded->Validate(Errors))
		{
			LogMessages(TEXT("Campaign is invalid"), Errors, ELogVerbosity::Error);
			Loaded = nullptr;
		}
		const_cast<UOutbreakCampaignSubsystem*>(this)->Campaign = Loaded;
	}
	return Campaign;
}

bool UOutbreakCampaignSubsystem::SetCampaign(UOutbreakCampaignDefinition* InCampaign)
{
	TArray<FText> Errors;
	if (!InCampaign || !InCampaign->Validate(Errors))
	{
		LogMessages(TEXT("Campaign rejected"), Errors, ELogVerbosity::Error);
		return false;
	}
	Campaign = InCampaign;
	Progress = FOutbreakCampaignProgress();
	bCampaignActive = false;
	ClearLaunch();
	OnCampaignChanged.Broadcast();
	return true;
}

bool UOutbreakCampaignSubsystem::StartNewCampaign()
{
	const UOutbreakCampaignDefinition* Definition = GetCampaign();
	if (!Definition)
	{
		UE_LOG(LogOutbreak, Error, TEXT("No valid campaign is set (Project Settings > Outbreak Campaign)."));
		return false;
	}
	Progress = FOutbreakCampaignProgress();
	Progress.CampaignId = Definition->GetResolvedCampaignId();
	bCampaignActive = true;
	ClearLaunch();
	UE_LOG(LogOutbreak, Log, TEXT("New campaign %s."), *Progress.CampaignId.ToString());
	Autosave();
	OnCampaignChanged.Broadcast();
	return true;
}

EOutbreakCampaignMissionStatus UOutbreakCampaignSubsystem::GetMissionStatus(FName MissionId) const
{
	const UOutbreakCampaignDefinition* Definition = GetCampaign();
	return Definition && bCampaignActive
		? OutbreakCampaignRules::GetMissionStatus(*Definition, Progress, Definition->FindMissionIndex(MissionId))
		: EOutbreakCampaignMissionStatus::Locked;
}

FName UOutbreakCampaignSubsystem::GetNextMissionId() const
{
	const UOutbreakCampaignDefinition* Definition = GetCampaign();
	return Definition && bCampaignActive ? OutbreakCampaignRules::FindNextMission(*Definition, Progress) : NAME_None;
}

bool UOutbreakCampaignSubsystem::IsCampaignComplete() const
{
	const UOutbreakCampaignDefinition* Definition = GetCampaign();
	return Definition && bCampaignActive && OutbreakCampaignRules::IsCampaignComplete(*Definition, Progress);
}

FText UOutbreakCampaignSubsystem::GetMissionDisplayName(FName MissionId) const
{
	const UOutbreakCampaignDefinition* Definition = GetCampaign();
	const FOutbreakCampaignMission* Entry = Definition ? Definition->FindMission(MissionId) : nullptr;
	const UOutbreakMissionDefinition* Mission = Entry ? Entry->Mission.LoadSynchronous() : nullptr;
	if (Mission && !Mission->DisplayName.IsEmpty())
	{
		return Mission->DisplayName;
	}
	return FText::FromName(MissionId);
}

// --- Launching -----------------------------------------------------------------------------------

bool UOutbreakCampaignSubsystem::PrepareLaunch(FName MissionId, bool bResumeFromCheckpoint)
{
	const UOutbreakCampaignDefinition* Definition = GetCampaign();
	if (!Definition || !bCampaignActive)
	{
		UE_LOG(LogOutbreak, Warning, TEXT("Cannot launch %s: no campaign is active."), *MissionId.ToString());
		return false;
	}
	const int32 Index = Definition->FindMissionIndex(MissionId);
	if (OutbreakCampaignRules::GetMissionStatus(*Definition, Progress, Index) == EOutbreakCampaignMissionStatus::Locked)
	{
		UE_LOG(LogOutbreak, Warning, TEXT("Cannot launch %s: unknown or locked."), *MissionId.ToString());
		return false;
	}
	const FOutbreakCampaignMission& Entry = Definition->Missions[Index];
	UOutbreakMissionDefinition* Mission = Entry.Mission.LoadSynchronous();
	if (!Mission || Entry.Map.IsNull())
	{
		UE_LOG(LogOutbreak, Error, TEXT("Cannot launch %s: its mission or map asset is missing."), *MissionId.ToString());
		return false;
	}
	if (Mission->GetResolvedMissionId() != MissionId)
	{
		UE_LOG(LogOutbreak, Warning, TEXT("Campaign mission %s uses %s, whose Mission Id is %s; progress is recorded under %s."),
			*MissionId.ToString(), *Mission->GetName(), *Mission->GetResolvedMissionId().ToString(), *MissionId.ToString());
	}

	const bool bHasCheckpoint = Progress.Checkpoint.MissionId == MissionId;
	if (!bResumeFromCheckpoint && Progress.Checkpoint.IsSet())
	{
		// A fresh run (of this or another mission) abandons the mission in progress.
		UE_LOG(LogOutbreak, Log, TEXT("Abandoning the checkpoint of %s."), *Progress.Checkpoint.MissionId.ToString());
		Progress.Checkpoint = FOutbreakMissionCheckpoint();
		Autosave();
	}

	LaunchedMission = Mission;
	LaunchedMissionId = MissionId;
	LaunchedMapPackage = Entry.Map.GetLongPackageName();
	bResumeCheckpoint = bResumeFromCheckpoint && bHasCheckpoint;
	return true;
}

bool UOutbreakCampaignSubsystem::LaunchMission(FName MissionId)
{
	if (!PrepareLaunch(MissionId, /*bResumeFromCheckpoint=*/ false))
	{
		return false;
	}
	UE_LOG(LogOutbreak, Log, TEXT("Launching campaign mission %s on %s."), *MissionId.ToString(), *LaunchedMapPackage);
	Travel(LaunchedMapPackage);
	return true;
}

bool UOutbreakCampaignSubsystem::ResumeCheckpoint()
{
	if (!HasCheckpoint() || !PrepareLaunch(Progress.Checkpoint.MissionId, /*bResumeFromCheckpoint=*/ true))
	{
		return false;
	}
	UE_LOG(LogOutbreak, Log, TEXT("Resuming campaign mission %s from its checkpoint."), *LaunchedMissionId.ToString());
	Travel(LaunchedMapPackage);
	return true;
}

bool UOutbreakCampaignSubsystem::ContinueCampaign()
{
	if (HasCheckpoint() && ResumeCheckpoint())
	{
		return true;
	}
	const FName Next = GetNextMissionId();
	return !Next.IsNone() && LaunchMission(Next);
}

void UOutbreakCampaignSubsystem::ReturnToFrontEnd()
{
	ClearLaunch();
	const UOutbreakCampaignSettings* Settings = GetDefault<UOutbreakCampaignSettings>();
	if (Settings->FrontEndMap.IsNull())
	{
		UE_LOG(LogOutbreak, Warning, TEXT("No front-end map is set (Project Settings > Outbreak Campaign); quitting instead."));
		UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
		if (World)
		{
			UKismetSystemLibrary::QuitGame(World, nullptr, EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/ false);
		}
		return;
	}
	Travel(Settings->FrontEndMap.GetLongPackageName());
}

void UOutbreakCampaignSubsystem::Travel(const FString& PackageName) const
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	if (World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer)
	{
		// Takes co-op clients along.
		World->ServerTravel(PackageName);
	}
	else
	{
		UGameplayStatics::OpenLevel(World, FName(*PackageName));
	}
}

void UOutbreakCampaignSubsystem::ClearLaunch()
{
	LaunchedMission = nullptr;
	LaunchedMissionId = NAME_None;
	LaunchedMapPackage.Reset();
	bResumeCheckpoint = false;
}

UOutbreakMissionDefinition* UOutbreakCampaignSubsystem::GetLaunchedMission(const FString& MapName) const
{
	if (!LaunchedMission || LaunchedMapPackage.IsEmpty())
	{
		return nullptr;
	}
	// MapName may carry a path and, in Play In Editor, a PIE prefix.
	const FString Loaded = FPackageName::GetShortName(UWorld::RemovePIEPrefix(MapName));
	return Loaded == FPackageName::GetShortName(LaunchedMapPackage) ? LaunchedMission.Get() : nullptr;
}

const FOutbreakMissionCheckpoint* UOutbreakCampaignSubsystem::GetCheckpointToResume() const
{
	return bResumeCheckpoint && Progress.Checkpoint.MissionId == LaunchedMissionId && !LaunchedMissionId.IsNone()
		? &Progress.Checkpoint : nullptr;
}

void UOutbreakCampaignSubsystem::SetResumeOnRestart(bool bResume)
{
	const bool bHasCheckpoint = Progress.Checkpoint.MissionId == LaunchedMissionId && !LaunchedMissionId.IsNone();
	bResumeCheckpoint = bResume && bHasCheckpoint;
	if (!bResume && bHasCheckpoint)
	{
		// Restarting from the briefing abandons the checkpoint, as a fresh launch does.
		Progress.Checkpoint = FOutbreakMissionCheckpoint();
		Autosave();
	}
}

void UOutbreakCampaignSubsystem::RecordCheckpoint(const FOutbreakMissionCheckpoint& Checkpoint)
{
	if (!bCampaignActive || LaunchedMissionId.IsNone())
	{
		return;
	}
	Progress.Checkpoint = Checkpoint;
	// Campaign bookkeeping uses the campaign's mission ID (see PrepareLaunch).
	Progress.Checkpoint.MissionId = LaunchedMissionId;
	UE_LOG(LogOutbreak, Log, TEXT("Checkpoint for %s (%d objectives completed, %.0f s)."), *LaunchedMissionId.ToString(),
		Checkpoint.CompletedObjectiveCount, Checkpoint.MissionTime);
	Autosave();
	OnCampaignChanged.Broadcast();
}

FOutbreakCampaignRecordResult UOutbreakCampaignSubsystem::RecordMissionResults(const FOutbreakMissionResults& Results)
{
	FOutbreakCampaignRecordResult Outcome;
	const UOutbreakCampaignDefinition* Definition = GetCampaign();
	if (!Definition || !bCampaignActive || LaunchedMissionId.IsNone())
	{
		return Outcome;
	}

	Outcome = OutbreakCampaignRules::RecordResults(*Definition, Progress, LaunchedMissionId, Results, FDateTime::UtcNow());
	if (!Outcome.bRecorded)
	{
		return Outcome;
	}
	UE_LOG(LogOutbreak, Log, TEXT("Campaign: %s %s; %d mission(s) unlocked%s."), *LaunchedMissionId.ToString(),
		Results.Outcome == EOutbreakMissionState::Completed ? TEXT("completed") : TEXT("failed"), Outcome.NewlyUnlocked.Num(),
		Outcome.bCampaignCompleted ? TEXT("; campaign complete") : TEXT(""));
	Autosave();
	OnCampaignChanged.Broadcast();
	if (Outcome.bCampaignCompleted)
	{
		OnCampaignCompleted.Broadcast();
	}
	return Outcome;
}

FOutbreakCampaignStatus UOutbreakCampaignSubsystem::BuildStatus(const FOutbreakCampaignRecordResult* LastResult) const
{
	FOutbreakCampaignStatus Status;
	const UOutbreakCampaignDefinition* Definition = GetCampaign();
	if (!Definition || !bCampaignActive || LaunchedMissionId.IsNone())
	{
		return Status;
	}
	Status.bCampaignMission = true;
	Status.CampaignName = Definition->DisplayName.IsEmpty() ? FText::FromName(Definition->GetResolvedCampaignId()) : Definition->DisplayName;
	const FName Next = OutbreakCampaignRules::FindNextMission(*Definition, Progress);
	Status.bHasNextMission = !Next.IsNone() && Next != LaunchedMissionId;
	if (Status.bHasNextMission)
	{
		Status.NextMissionName = GetMissionDisplayName(Next);
	}
	Status.bHasCheckpoint = Progress.Checkpoint.MissionId == LaunchedMissionId;
	Status.bCampaignComplete = OutbreakCampaignRules::IsCampaignComplete(*Definition, Progress);
	Status.CompletionText = Definition->CompletionText;
	if (LastResult)
	{
		Status.bCampaignJustCompleted = LastResult->bCampaignCompleted;
		for (const FName Unlocked : LastResult->NewlyUnlocked)
		{
			Status.UnlockedMissionNames.Add(GetMissionDisplayName(Unlocked));
		}
	}
	return Status;
}

// --- Save slots ----------------------------------------------------------------------------------

int32 UOutbreakCampaignSubsystem::GetSlotCount() const
{
	return 1 + FMath::Max(0, GetDefault<UOutbreakCampaignSettings>()->ManualSlotCount);
}

FString UOutbreakCampaignSubsystem::GetSlotName(int32 SlotIndex) const
{
	const FString& Prefix = GetDefault<UOutbreakCampaignSettings>()->SlotNamePrefix;
	return SlotIndex == 0 ? Prefix + TEXT("Auto") : FString::Printf(TEXT("%s%d"), *Prefix, SlotIndex);
}

UOutbreakCampaignSaveGame* UOutbreakCampaignSubsystem::CreateSaveObject() const
{
	UOutbreakCampaignSaveGame* Save = Cast<UOutbreakCampaignSaveGame>(UGameplayStatics::CreateSaveGameObject(UOutbreakCampaignSaveGame::StaticClass()));
	if (!Save)
	{
		return nullptr;
	}
	Save->SaveVersion = OutbreakSaveVersion::Current;
	Save->Progress = Progress;
	Save->SavedAt = FDateTime::UtcNow();
	if (const UOutbreakCampaignDefinition* Definition = GetCampaign())
	{
		Save->CampaignName = Definition->DisplayName;
		const FName Current = Progress.Checkpoint.IsSet() ? Progress.Checkpoint.MissionId : OutbreakCampaignRules::FindNextMission(*Definition, Progress);
		Save->CurrentMissionName = Current.IsNone() ? FText::GetEmpty() : GetMissionDisplayName(Current);
	}
	return Save;
}

bool UOutbreakCampaignSubsystem::SaveToSlot(int32 SlotIndex)
{
	const UOutbreakCampaignDefinition* Definition = GetCampaign();
	if (!Definition || !bCampaignActive || SlotIndex < 0 || SlotIndex >= GetSlotCount())
	{
		LastSaveMessage = LOCTEXT("SaveNothing", "Nothing to save.");
		return false;
	}

	TArray<FText> Errors;
	if (!OutbreakSaveValidation::ValidateForSave(Progress, *Definition, Errors))
	{
		LogMessages(TEXT("Save refused"), Errors, ELogVerbosity::Error);
		LastSaveMessage = FText::Format(LOCTEXT("SaveRefused", "Save refused: {0}"), Errors[0]);
		return false;
	}

	UOutbreakCampaignSaveGame* Save = CreateSaveObject();
	const FString SlotName = GetSlotName(SlotIndex);
	if (!Save || !UGameplayStatics::SaveGameToSlot(Save, SlotName, SaveUserIndex))
	{
		UE_LOG(LogOutbreak, Error, TEXT("Could not write save slot %s."), *SlotName);
		LastSaveMessage = LOCTEXT("SaveFailed", "The save could not be written.");
		return false;
	}
	UE_LOG(LogOutbreak, Log, TEXT("Saved the campaign to %s."), *SlotName);
	LastSaveMessage = SlotIndex == 0 ? FText::GetEmpty() : FText::Format(LOCTEXT("Saved", "Saved to slot {0}."), SlotIndex);
	OnCampaignChanged.Broadcast();
	return true;
}

void UOutbreakCampaignSubsystem::Autosave()
{
	if (GetDefault<UOutbreakCampaignSettings>()->bAutosave)
	{
		SaveToSlot(0);
	}
}

bool UOutbreakCampaignSubsystem::ApplyLoadedSave(USaveGame* Loaded, TArray<FText>& OutErrors)
{
	const UOutbreakCampaignDefinition* Definition = GetCampaign();
	if (!Definition)
	{
		OutErrors.Add(LOCTEXT("LoadNoCampaign", "No valid campaign is set."));
		return false;
	}

	TArray<FText> Warnings;
	if (!OutbreakSaveValidation::ValidateLoaded(Loaded, *Definition, OutErrors, Warnings))
	{
		return false;
	}
	LogMessages(TEXT("Save repaired"), Warnings, ELogVerbosity::Warning);

	Progress = CastChecked<UOutbreakCampaignSaveGame>(Loaded)->Progress;
	bCampaignActive = true;
	ClearLaunch();
	OnCampaignChanged.Broadcast();
	return true;
}

bool UOutbreakCampaignSubsystem::LoadFromSlot(int32 SlotIndex)
{
	const FString SlotName = GetSlotName(SlotIndex);
	if (SlotIndex < 0 || SlotIndex >= GetSlotCount() || !UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex))
	{
		LastSaveMessage = LOCTEXT("LoadEmpty", "That slot is empty.");
		return false;
	}

	TArray<FText> Errors;
	if (!ApplyLoadedSave(UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex), Errors))
	{
		LogMessages(*FString::Printf(TEXT("Cannot load %s"), *SlotName), Errors, ELogVerbosity::Error);
		LastSaveMessage = FText::Format(LOCTEXT("LoadRefused", "Cannot load: {0}"), Errors.IsEmpty() ? FText::GetEmpty() : Errors[0]);
		return false;
	}
	UE_LOG(LogOutbreak, Log, TEXT("Loaded the campaign from %s."), *SlotName);
	LastSaveMessage = FText::GetEmpty();
	return true;
}

bool UOutbreakCampaignSubsystem::DeleteSlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= GetSlotCount())
	{
		return false;
	}
	const bool bDeleted = UGameplayStatics::DeleteGameInSlot(GetSlotName(SlotIndex), SaveUserIndex);
	OnCampaignChanged.Broadcast();
	return bDeleted;
}

FOutbreakSaveSlotSummary UOutbreakCampaignSubsystem::SummarizeSave(int32 SlotIndex, const UOutbreakCampaignSaveGame& Save) const
{
	FOutbreakSaveSlotSummary Summary;
	Summary.SlotIndex = SlotIndex;
	Summary.bExists = true;
	Summary.SavedAt = Save.SavedAt;
	Summary.CampaignName = Save.CampaignName;
	Summary.CurrentMissionName = Save.CurrentMissionName;
	Summary.bHasCheckpoint = Save.Progress.Checkpoint.IsSet();
	if (const UOutbreakCampaignDefinition* Definition = GetCampaign())
	{
		OutbreakCampaignRules::CountRequired(*Definition, Save.Progress, Summary.CompletedMissions, Summary.RequiredMissions);
		Summary.bCampaignComplete = OutbreakCampaignRules::IsCampaignComplete(*Definition, Save.Progress);
	}
	return Summary;
}

FOutbreakSaveSlotSummary UOutbreakCampaignSubsystem::GetSlotSummary(int32 SlotIndex) const
{
	FOutbreakSaveSlotSummary Summary;
	Summary.SlotIndex = SlotIndex;
	const FString SlotName = GetSlotName(SlotIndex);
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex))
	{
		return Summary;
	}

	USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex);
	const UOutbreakCampaignDefinition* Definition = GetCampaign();
	TArray<FText> Errors;
	TArray<FText> Warnings;
	if (!Definition || !OutbreakSaveValidation::ValidateLoaded(Loaded, *Definition, Errors, Warnings))
	{
		Summary.bExists = true;
		Summary.bUnreadable = true;
		return Summary;
	}
	return SummarizeSave(SlotIndex, *CastChecked<UOutbreakCampaignSaveGame>(Loaded));
}

#undef LOCTEXT_NAMESPACE
