#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Missions/OutbreakMissionTypes.h"

#include "OutbreakMissionSummaryWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOutbreakMissionSummaryActionSignature);

/**
 * Mission completion screen (Phase 11): outcome, mission time, objectives and squad report from
 * FOutbreakMissionResults, with Retry and Quit. (Continue to the next mission arrives with the
 * Phase 14 campaign.)
 *
 * Designers subclass it as a Widget Blueprint with the bound names below (all optional). Without a
 * designer layout it builds a plain default layout.
 */
UCLASS(Blueprintable)
class OUTBREAK_API UOutbreakMissionSummaryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Mission Summary")
	void ShowResults(const FOutbreakMissionResults& Results);

	UPROPERTY(BlueprintAssignable, Category = "Mission Summary")
	FOutbreakMissionSummaryActionSignature OnRetryRequested;

	UPROPERTY(BlueprintAssignable, Category = "Mission Summary")
	FOutbreakMissionSummaryActionSignature OnQuitRequested;

protected:

	virtual void NativeOnInitialized() override;

	/** After ShowResults, for Blueprint presentation. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Mission Summary")
	void OnResultsShown(const FOutbreakMissionResults& Results);

	UPROPERTY(BlueprintReadOnly, Category = "Mission Summary", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Mission Summary", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryText;

	UPROPERTY(BlueprintReadOnly, Category = "Mission Summary", meta = (BindWidgetOptional))
	TObjectPtr<UButton> RetryButton;

	UPROPERTY(BlueprintReadOnly, Category = "Mission Summary", meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

private:

	void BuildDefaultLayout();

	UFUNCTION()
	void HandleRetryClicked();

	UFUNCTION()
	void HandleQuitClicked();
};
