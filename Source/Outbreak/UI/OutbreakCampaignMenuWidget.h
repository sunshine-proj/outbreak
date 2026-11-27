#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "OutbreakCampaignMenuWidget.generated.h"

class UButton;
class UOutbreakCampaignSubsystem;
class UOutbreakMenuButton;
class UTextBlock;
class UVerticalBox;

/**
 * The campaign screen (Phase 14 campaign briefing): the campaign briefing and progress, the
 * mission list with unlock state and records, Continue / New Campaign, the save slots (load,
 * save, delete) and the player settings. Shown by AOutbreakFrontEndPlayerController.
 *
 * Designers can subclass it as a Widget Blueprint with the bound names below (all optional) and
 * call UOutbreakCampaignSubsystem directly; without a designer layout it builds a plain default
 * layout. It refreshes whenever the campaign changes.
 */
UCLASS(Blueprintable)
class OUTBREAK_API UOutbreakCampaignMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Re-reads the campaign, slots and settings. */
	UFUNCTION(BlueprintCallable, Category = "Campaign Menu")
	void Refresh();

protected:

	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	/** After Refresh, for Blueprint presentation. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Campaign Menu")
	void OnMenuRefreshed();

	UPROPERTY(BlueprintReadOnly, Category = "Campaign Menu", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign Menu", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BriefingText;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign Menu", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ProgressText;

	/** Results of the last action (saved, refused, cannot load...). */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign Menu", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign Menu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign Menu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> NewCampaignButton;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign Menu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	/** Filled with one row per campaign mission. */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign Menu", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> MissionList;

	/** Filled with one row per save slot. */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign Menu", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> SlotList;

	/** Filled with the player settings rows. */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign Menu", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> SettingsList;

private:

	enum class EAction : int32
	{
		Launch,
		Load,
		Save,
		Delete,
		SensitivityDown,
		SensitivityUp,
		ToggleInvert,
		FieldOfViewDown,
		FieldOfViewUp,
		ToggleSubtitles,
		ApplySettings
	};

	void BuildDefaultLayout();
	void RebuildMissionRows();
	void RebuildSlotRows();
	void RebuildSettingsRows();
	UTextBlock* MakeText(const FText& Text) const;
	UOutbreakMenuButton* MakeButton(const FText& Label, EAction Action, int32 Index);
	void SetMessage(const FText& Message);
	UOutbreakCampaignSubsystem* GetCampaign() const;

	UFUNCTION()
	void HandleCampaignChanged();

	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleNewCampaignClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleRowButton(UOutbreakMenuButton* Button);

	/** New Campaign over existing progress asks for a second click. */
	bool bConfirmNewCampaign = false;

	/** Delete asks for a second click on the same slot. */
	int32 ConfirmDeleteSlot = INDEX_NONE;

	FText LastMessage;
};
