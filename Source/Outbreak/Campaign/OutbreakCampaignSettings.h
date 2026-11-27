#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"

#include "OutbreakCampaignSettings.generated.h"

class UOutbreakCampaignDefinition;
class UWorld;

/**
 * Project-wide campaign and save settings (Phase 14): Project Settings → Game → Outbreak Campaign,
 * stored in Config/DefaultGame.ini.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Outbreak Campaign"))
class OUTBREAK_API UOutbreakCampaignSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** The campaign the front end offers. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Campaign")
	TSoftObjectPtr<UOutbreakCampaignDefinition> Campaign;

	/** The campaign screen's level (game mode AOutbreakFrontEndGameMode). Main Menu on the summary screen returns here. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Campaign")
	TSoftObjectPtr<UWorld> FrontEndMap;

	/** Manual save slots besides the autosave. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Saving", meta = (ClampMin = "0", ClampMax = "20"))
	int32 ManualSlotCount = 3;

	/** Slot file names: <prefix>Auto, <prefix>1, <prefix>2, ... */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Saving")
	FString SlotNamePrefix = TEXT("Campaign_");

	/** Write the autosave after every ended mission and every checkpoint. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Saving")
	bool bAutosave = true;
};
