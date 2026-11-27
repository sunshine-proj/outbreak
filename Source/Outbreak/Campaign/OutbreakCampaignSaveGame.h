#pragma once

#include "CoreMinimal.h"
#include "Campaign/OutbreakCampaignTypes.h"
#include "GameFramework/SaveGame.h"

#include "OutbreakCampaignSaveGame.generated.h"

class UOutbreakCampaignDefinition;

/** Save format versions. Add a value (and a migration in OutbreakSaveValidation::ValidateLoaded) when the format changes. */
namespace OutbreakSaveVersion
{
	/** Phase 14: campaign progress, mission records, one checkpoint. */
	constexpr int32 Initial = 1;
	constexpr int32 Current = Initial;
}

/**
 * One campaign save slot (Phase 14 SaveGame architecture, D-023). Holds the campaign progress
 * (records and checkpoint) and the slot's display information. Written and read by
 * UOutbreakCampaignSubsystem; every save and load goes through OutbreakSaveValidation.
 *
 * Settings are not here: they persist through UOutbreakGameUserSettings (GameUserSettings.ini).
 */
UCLASS()
class OUTBREAK_API UOutbreakCampaignSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(SaveGame)
	int32 SaveVersion = OutbreakSaveVersion::Current;

	UPROPERTY(SaveGame)
	FOutbreakCampaignProgress Progress;

	/** When the slot was written (UTC). */
	UPROPERTY(SaveGame)
	FDateTime SavedAt;

	/** For the slot list, so it can be shown without the campaign asset. */
	UPROPERTY(SaveGame)
	FText CampaignName;

	/** Mission the slot continues with (the checkpoint's, else the next one), for the slot list. */
	UPROPERTY(SaveGame)
	FText CurrentMissionName;
};

/** What the save slot list shows. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakSaveSlotSummary
{
	GENERATED_BODY()

	/** 0 = the autosave, 1.. = manual slots. */
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 SlotIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	bool bExists = false;

	/** Exists but cannot be loaded (corrupt, from a newer version, or another campaign). */
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	bool bUnreadable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FDateTime SavedAt;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FText CampaignName;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FText CurrentMissionName;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 CompletedMissions = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 RequiredMissions = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	bool bHasCheckpoint = false;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	bool bCampaignComplete = false;
};

/**
 * Save and load validation (Phase 14). Pure functions over the save object and the campaign, so
 * they are testable without disk access.
 */
namespace OutbreakSaveValidation
{
	/**
	 * Before writing: the progress belongs to Campaign, records name its missions without duplicates
	 * and with consistent counts, and a checkpoint names an unlocked, uncompleted mission with a
	 * squad within the project's size limits. Returns false (nothing should be written) with reasons.
	 */
	OUTBREAK_API bool ValidateForSave(const FOutbreakCampaignProgress& Progress, const UOutbreakCampaignDefinition& Campaign, TArray<FText>& OutErrors);

	/**
	 * After reading: rejects (returns false, with errors) a missing or foreign object, a version
	 * newer than this build or invalid, and another campaign's save. Otherwise repairs in place what
	 * can be repaired, appending a warning for each: records of missions no longer in the campaign
	 * are dropped, duplicates merged, negative counts and times clamped, and an unusable checkpoint
	 * discarded. Older versions are migrated here.
	 */
	OUTBREAK_API bool ValidateLoaded(USaveGame* Loaded, const UOutbreakCampaignDefinition& Campaign,
		TArray<FText>& OutErrors, TArray<FText>& OutWarnings);
}
