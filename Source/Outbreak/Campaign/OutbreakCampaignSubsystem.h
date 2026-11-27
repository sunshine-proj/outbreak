#pragma once

#include "CoreMinimal.h"
#include "Campaign/OutbreakCampaignSaveGame.h"
#include "Campaign/OutbreakCampaignTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "OutbreakCampaignSubsystem.generated.h"

class UOutbreakCampaignDefinition;
class UOutbreakMissionDefinition;
class USaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOutbreakCampaignChangedSignature);

/**
 * The running campaign (Phase 14, D-023). Lives on the game instance, so it survives level travel:
 * the front end starts or loads a campaign here and launches a mission; the mission level's game
 * mode asks it which mission to run, reports checkpoints and results back, and the summary's
 * Continue / Retry / Main Menu travel through it.
 *
 * Progress is in memory; it reaches disk through the save slots: slot 0 is the autosave (written
 * after every ended campaign mission and checkpoint), 1..N are manual slots. Every write and read
 * is validated (OutbreakSaveValidation). Runs on the server / standalone game; clients see the
 * campaign through AOutbreakGameState.
 */
UCLASS()
class OUTBREAK_API UOutbreakCampaignSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// --- Campaign ---

	/** The campaign in use: one set with SetCampaign, else the project's (Outbreak Campaign settings). May be null. */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	UOutbreakCampaignDefinition* GetCampaign() const;

	/** Uses InCampaign instead of the project's campaign (tests, additional campaigns). Ends the current session. Returns false if invalid. */
	bool SetCampaign(UOutbreakCampaignDefinition* InCampaign);

	/** A campaign was started or loaded in this session. */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool IsCampaignActive() const { return bCampaignActive; }

	/** Fresh progress for the campaign, written to the autosave. Returns false without a valid campaign. */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	bool StartNewCampaign();

	const FOutbreakCampaignProgress& GetProgress() const { return Progress; }

	UFUNCTION(BlueprintPure, Category = "Campaign")
	EOutbreakCampaignMissionStatus GetMissionStatus(FName MissionId) const;

	/** See OutbreakCampaignRules::FindNextMission. None when the campaign is complete or blocked. */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	FName GetNextMissionId() const;

	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool IsCampaignComplete() const;

	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool HasCheckpoint() const { return bCampaignActive && Progress.Checkpoint.IsSet(); }

	/** Display name of a campaign mission (loads its definition), or its ID. */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	FText GetMissionDisplayName(FName MissionId) const;

	// --- Launching ---

	/** Travels to MissionId's level to play it from the briefing. Refused for locked or unknown missions. */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	bool LaunchMission(FName MissionId);

	/** Travels to the checkpointed mission and resumes it there. */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	bool ResumeCheckpoint();

	/** Resumes the checkpoint if there is one, else launches the next mission. */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	bool ContinueCampaign();

	/** Ends the mission session and travels to the front-end map. */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void ReturnToFrontEnd();

	/** Sets the launch up without travelling (LaunchMission's first half; tests use it directly). */
	bool PrepareLaunch(FName MissionId, bool bResumeFromCheckpoint);

	// --- The mission level (game mode, server) ---

	/** The launched mission when MapName is its level; null when this level was not launched by the campaign. */
	UOutbreakMissionDefinition* GetLaunchedMission(const FString& MapName) const;

	FName GetLaunchedMissionId() const { return LaunchedMissionId; }

	/** The checkpoint to resume this launch from, or null (fresh start). */
	const FOutbreakMissionCheckpoint* GetCheckpointToResume() const;

	/** Whether the next replay of the launched mission (the summary's Retry) resumes its checkpoint. */
	void SetResumeOnRestart(bool bResume);

	/** The launched mission reached a checkpoint: kept in the progress and autosaved. */
	void RecordCheckpoint(const FOutbreakMissionCheckpoint& Checkpoint);

	/** The launched mission ended: records, unlocking, completion; autosaved. */
	FOutbreakCampaignRecordResult RecordMissionResults(const FOutbreakMissionResults& Results);

	/** For the summary screen. LastResult adds what the latest result changed. */
	FOutbreakCampaignStatus BuildStatus(const FOutbreakCampaignRecordResult* LastResult = nullptr) const;

	// --- Save slots ---

	/** The autosave plus the manual slots. */
	UFUNCTION(BlueprintPure, Category = "Campaign|Save")
	int32 GetSlotCount() const;

	FString GetSlotName(int32 SlotIndex) const;

	/** Validates the progress and writes it to SlotIndex (0 = autosave). Returns false, logged, when refused or the write fails. */
	UFUNCTION(BlueprintCallable, Category = "Campaign|Save")
	bool SaveToSlot(int32 SlotIndex);

	/** Reads, validates and applies SlotIndex. Returns false (progress unchanged), logged, when the save is unusable. */
	UFUNCTION(BlueprintCallable, Category = "Campaign|Save")
	bool LoadFromSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Campaign|Save")
	bool DeleteSlot(int32 SlotIndex);

	/** Reads SlotIndex for the slot list (without applying it). */
	UFUNCTION(BlueprintCallable, Category = "Campaign|Save")
	FOutbreakSaveSlotSummary GetSlotSummary(int32 SlotIndex) const;

	/** The progress as a save object (validated by the caller). */
	UOutbreakCampaignSaveGame* CreateSaveObject() const;

	/** Validates Loaded and, if usable, makes it the progress. Warnings are logged. */
	bool ApplyLoadedSave(USaveGame* Loaded, TArray<FText>& OutErrors);

	/** Messages from the last save or load, for the campaign screen. */
	const FText& GetLastSaveMessage() const { return LastSaveMessage; }

	/** Progress, campaign or slots changed. */
	UPROPERTY(BlueprintAssignable, Category = "Campaign")
	FOutbreakCampaignChangedSignature OnCampaignChanged;

	/** Every required mission has now been completed. */
	UPROPERTY(BlueprintAssignable, Category = "Campaign")
	FOutbreakCampaignChangedSignature OnCampaignCompleted;

private:

	void Autosave();
	void Travel(const FString& PackageName) const;
	void ClearLaunch();
	FOutbreakSaveSlotSummary SummarizeSave(int32 SlotIndex, const UOutbreakCampaignSaveGame& Save) const;

	/** Set by SetCampaign; else the project's campaign is loaded on demand. */
	UPROPERTY(Transient)
	TObjectPtr<UOutbreakCampaignDefinition> Campaign;

	UPROPERTY(Transient)
	TObjectPtr<UOutbreakMissionDefinition> LaunchedMission;

	FOutbreakCampaignProgress Progress;
	bool bCampaignActive = false;

	FName LaunchedMissionId;
	FString LaunchedMapPackage;
	bool bResumeCheckpoint = false;

	FText LastSaveMessage;
};
