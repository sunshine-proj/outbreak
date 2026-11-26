#include "UI/OutbreakMissionSummaryWidget.h"

#include "Blueprint/WidgetTree.h"
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

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	RetryButton = UOutbreakMenuButton::Create(*WidgetTree, LOCTEXT("Retry", "  RETRY  "), 0);
	QuitButton = UOutbreakMenuButton::Create(*WidgetTree, LOCTEXT("Quit", "  QUIT  "), 0);
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

void UOutbreakMissionSummaryWidget::HandleRetryClicked()
{
	OnRetryRequested.Broadcast();
}

void UOutbreakMissionSummaryWidget::HandleQuitClicked()
{
	OnQuitRequested.Broadcast();
}

#undef LOCTEXT_NAMESPACE
