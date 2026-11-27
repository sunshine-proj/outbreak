#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPtr.h"

#include "OutbreakCampaignDefinition.generated.h"

class UOutbreakMissionDefinition;
class UWorld;

/** When a campaign mission becomes available (Phase 14 mission unlocking). */
UENUM(BlueprintType)
enum class EOutbreakMissionUnlock : uint8
{
	/** After the nearest earlier required mission is completed (available at once if there is none). */
	AfterPrevious,
	/** From the start of the campaign. */
	AtCampaignStart,
	/** After every mission listed in RequiredMissions is completed. */
	AfterMissions
};

/** One mission of a campaign: which mission runs on which level, and when it unlocks. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakCampaignMission
{
	GENERATED_BODY()

	/**
	 * Identifier saved in campaign progress. Must be unique in the campaign and should match the
	 * mission definition's Mission Id (data validation checks it). Never rename it after release:
	 * saves refer to it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FName MissionId;

	/** Runs instead of the level game mode's own mission. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	TSoftObjectPtr<UOutbreakMissionDefinition> Mission;

	/** The level the mission is played on. Its game mode must be an Outbreak game mode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	TSoftObjectPtr<UWorld> Map;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlocking")
	EOutbreakMissionUnlock Unlock = EOutbreakMissionUnlock::AfterPrevious;

	/** Missions (listed earlier in the campaign) that must be completed first. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlocking",
		meta = (EditCondition = "Unlock == EOutbreakMissionUnlock::AfterMissions", EditConditionHides))
	TArray<FName> RequiredMissions;

	/** A side mission: not needed to complete the campaign. Required missions cannot depend on it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	bool bOptional = false;
};

/**
 * A campaign (Phase 14): an ordered list of missions with unlock rules, and the campaign
 * briefing shown on the campaign screen. The campaign is complete when every required mission
 * has been completed once. Set it in Project Settings → Game → Outbreak Campaign.
 */
UCLASS(BlueprintType, Const)
class OUTBREAK_API UOutbreakCampaignDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** Structural rules: IDs, assets set, unlock references, at least one required and one starting mission. */
	bool Validate(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	/** Validate plus loading each mission to check it is valid and its Mission Id matches the entry. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/** Saves refer to the campaign by this ID. Empty = the asset name. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campaign")
	FName CampaignId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campaign")
	FText DisplayName;

	/** The campaign briefing: the situation, shown above the mission list. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campaign", meta = (MultiLine = "true"))
	FText Briefing;

	/** Shown once every required mission is completed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campaign", meta = (MultiLine = "true"))
	FText CompletionText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Campaign")
	TArray<FOutbreakCampaignMission> Missions;

	FName GetResolvedCampaignId() const;

	/** Index of MissionId in Missions, or INDEX_NONE. */
	int32 FindMissionIndex(FName MissionId) const;

	const FOutbreakCampaignMission* FindMission(FName MissionId) const;
};
