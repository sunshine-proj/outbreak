#include "UI/OutbreakMissionSummaryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Campaign/OutbreakCampaignText.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Missions/OutbreakMissionText.h"
#include "UI/OutbreakMenuButton.h"

#define LOCTEXT_NAMESPACE "OutbreakMissionSummary"

void UOutbreakMissionSummaryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	if (RetryButton)
	{
		RetryButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRetryClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleQuitClicked);
	}
	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleContinueClicked);
		ContinueButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CheckpointButton)
	{
		CheckpointButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCheckpointClicked);
		CheckpointButton->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UOutbreakMissionSummaryWidget::BuildDefaultLayout()
{
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	Background->SetPadding(FMargin(60.0f, 40.0f));
	WidgetTree->RootWidget = Background;

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Scroll"));
	Background->SetContent(Scroll);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
	Scroll->AddChild(Content);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	Content->AddChildToVerticalBox(TitleText);

	SummaryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SummaryText"));
	SummaryText->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(SummaryText)->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));

	CampaignText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CampaignText"));
	CampaignText->SetAutoWrapText(true);
	CampaignText->SetVisibility(ESlateVisibility::Collapsed);
	Content->AddChildToVerticalBox(CampaignText)->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	ContinueButton = UOutbreakMenuButton::Create(*WidgetTree, LOCTEXT("Continue", "  CONTINUE  "), 0);
	CheckpointButton = UOutbreakMenuButton::Create(*WidgetTree, LOCTEXT("Checkpoint", "  LAST CHECKPOINT  "), 0);
	RetryButton = UOutbreakMenuButton::Create(*WidgetTree, LOCTEXT("Retry", "  RETRY  "), 0);
	QuitButton = UOutbreakMenuButton::Create(*WidgetTree, LOCTEXT("Quit", "  QUIT  "), 0);
	Buttons->AddChildToHorizontalBox(ContinueButton)->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
	Buttons->AddChildToHorizontalBox(CheckpointButton)->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
	Buttons->AddChildToHorizontalBox(RetryButton);
	Buttons->AddChildToHorizontalBox(QuitButton)->SetPadding(FMargin(16.0f, 0.0f, 0.0f, 0.0f));
	Content->AddChildToVerticalBox(Buttons)->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
}

void UOutbreakMissionSummaryWidget::ShowResults(const FOutbreakMissionResults& Results)
{
	if (TitleText)
	{
		TitleText->SetText(OutbreakMissionText::BuildResultTitle(Results));
	}
	if (SummaryText)
	{
		SummaryText->SetText(OutbreakMissionText::BuildSummary(Results));
	}
	OnResultsShown(Results);
}

void UOutbreakMissionSummaryWidget::ShowCampaignStatus(const FOutbreakCampaignStatus& Status, const FOutbreakMissionResults& Results)
{
	const bool bCompleted = Results.Outcome == EOutbreakMissionState::Completed;
	const OutbreakCampaignText::FSummaryOptions Options = OutbreakCampaignText::GetSummaryOptions(Status, Results);

	if (CampaignText)
	{
		const FText Text = OutbreakCampaignText::BuildSummaryCampaignText(Status, Results);
		CampaignText->SetText(Text);
		CampaignText->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (ContinueButton)
	{
		ContinueButton->SetVisibility(Options.bContinue ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (CheckpointButton)
	{
		CheckpointButton->SetVisibility(Options.bCheckpoint ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	// Default-layout buttons can be relabelled; Widget Blueprint buttons keep their designer labels.
	if (UOutbreakMenuButton* Retry = Cast<UOutbreakMenuButton>(RetryButton))
	{
		Retry->SetLabel(Status.bCampaignMission && (Options.bCheckpoint || bCompleted)
			? (bCompleted ? LOCTEXT("Replay", "  REPLAY  ") : LOCTEXT("RestartMission", "  RESTART MISSION  "))
			: LOCTEXT("Retry", "  RETRY  "));
	}
	if (UOutbreakMenuButton* Quit = Cast<UOutbreakMenuButton>(QuitButton))
	{
		Quit->SetLabel(Status.bCampaignMission ? LOCTEXT("MainMenu", "  MAIN MENU  ") : LOCTEXT("Quit", "  QUIT  "));
	}
}

void UOutbreakMissionSummaryWidget::HandleContinueClicked()
{
	OnContinueRequested.Broadcast();
}

void UOutbreakMissionSummaryWidget::HandleCheckpointClicked()
{
	OnCheckpointRequested.Broadcast();
}

void UOutbreakMissionSummaryWidget::HandleRetryClicked()
{
	OnRetryRequested.Broadcast();
}

void UOutbreakMissionSummaryWidget::HandleQuitClicked()
{
	OnQuitRequested.Broadcast();
}

#undef LOCTEXT_NAMESPACE
