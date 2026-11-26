#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Squad/OutbreakSquadSelection.h"
#include "Squad/OutbreakSquadTypes.h"

#include "OutbreakBriefingWidget.generated.h"

class UButton;
class UOutbreakMenuButton;
class UOutbreakMissionDefinition;
class UOutbreakSquadConfiguration;
class UPanelWidget;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakDeployRequestedSignature, const FOutbreakSquadSelection&, Selection);

/**
 * Mission briefing screen (Phase 11): mission name, description, briefing and objectives, then
 * squad selection (size and which roster soldiers) and equipment selection (a loadout per member),
 * and a Deploy button.
 *
 * Holds the selection locally and hands it to OnDeployRequested; the server validates it.
 * Designers subclass it as a Widget Blueprint and add widgets with the bound names below (all
 * optional; MemberList receives one generated row per member). Without a designer layout it builds
 * a plain default layout.
 */
UCLASS(Blueprintable)
class OUTBREAK_API UOutbreakBriefingWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Shows Mission and starts from the default selection for Configuration (null = project default). */
	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void ShowMission(UOutbreakMissionDefinition* Mission, UOutbreakSquadConfiguration* Configuration);

	UFUNCTION(BlueprintPure, Category = "Briefing")
	const FOutbreakSquadSelection& GetSelection() const { return Selection; }

	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void ChangeSquadSize(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void CycleTeammate(int32 Slot, int32 Direction);

	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void CycleLoadout(int32 MemberIndex, int32 Direction);

	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void Deploy();

	UPROPERTY(BlueprintAssignable, Category = "Briefing")
	FOutbreakDeployRequestedSignature OnDeployRequested;

protected:

	virtual void NativeOnInitialized() override;

	/** After every selection change, for Blueprint presentation. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Briefing")
	void OnSelectionChanged();

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MissionNameText;

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BriefingText;

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SquadSizeText;

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SquadSizeDecreaseButton;

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SquadSizeIncreaseButton;

	/** One row per member is generated into this panel. */
	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> MemberList;

	UPROPERTY(BlueprintReadOnly, Category = "Briefing", meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeployButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Briefing")
	TObjectPtr<UOutbreakMissionDefinition> MissionDefinition;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Briefing")
	TObjectPtr<UOutbreakSquadConfiguration> SquadConfiguration;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Briefing")
	FOutbreakSquadSelection Selection;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Briefing")
	FOutbreakSquadSizeLimits SizeLimits;

private:

	enum EAction : int32 { PreviousTeammate, NextTeammate, PreviousLoadout, NextLoadout };

	void BuildDefaultLayout();
	void Refresh();
	void RebuildMemberRows();

	UFUNCTION()
	void HandleMemberButton(UOutbreakMenuButton* Button);

	UFUNCTION()
	void HandleDecreaseClicked();

	UFUNCTION()
	void HandleIncreaseClicked();

	UFUNCTION()
	void HandleDeployClicked();
};
