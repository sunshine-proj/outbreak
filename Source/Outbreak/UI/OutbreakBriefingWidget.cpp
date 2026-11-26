#include "UI/OutbreakBriefingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "Missions/OutbreakMissionText.h"
#include "Squad/OutbreakSquadConfiguration.h"
#include "UI/OutbreakMenuButton.h"

#define LOCTEXT_NAMESPACE "OutbreakBriefing"

void UOutbreakBriefingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}

	if (SquadSizeDecreaseButton)
	{
		SquadSizeDecreaseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDecreaseClicked);
	}
	if (SquadSizeIncreaseButton)
	{
		SquadSizeIncreaseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleIncreaseClicked);
	}
	if (DeployButton)
	{
		DeployButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDeployClicked);
	}
}

void UOutbreakBriefingWidget::BuildDefaultLayout()
{
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	Background->SetPadding(FMargin(60.0f, 40.0f));
	WidgetTree->RootWidget = Background;

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Scroll"));
	Background->SetContent(Scroll);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
	Scroll->AddChild(Content);

	auto AddRow = [Content](UWidget* Widget, float TopPadding)
	{
		Content->AddChildToVerticalBox(Widget)->SetPadding(FMargin(0.0f, TopPadding, 0.0f, 0.0f));
	};
	auto MakeText = [this](FName Name, const FText& Text)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TextBlock->SetText(Text);
		TextBlock->SetAutoWrapText(true);
		return TextBlock;
	};

	MissionNameText = MakeText(TEXT("MissionNameText"), FText::GetEmpty());
	AddRow(MissionNameText, 0.0f);

	BriefingText = MakeText(TEXT("BriefingText"), FText::GetEmpty());
	AddRow(BriefingText, 16.0f);

	AddRow(MakeText(TEXT("SquadHeader"), LOCTEXT("SquadHeader", "SQUAD AND EQUIPMENT")), 32.0f);

	UHorizontalBox* SizeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SizeRow"));
	SquadSizeDecreaseButton = UOutbreakMenuButton::Create(*WidgetTree, LOCTEXT("Decrease", " - "), 0);
	SquadSizeText = MakeText(TEXT("SquadSizeText"), FText::GetEmpty());
	SquadSizeIncreaseButton = UOutbreakMenuButton::Create(*WidgetTree, LOCTEXT("Increase", " + "), 0);
	SizeRow->AddChildToHorizontalBox(SquadSizeDecreaseButton);
	SizeRow->AddChildToHorizontalBox(SquadSizeText)->SetPadding(FMargin(12.0f, 4.0f));
	SizeRow->AddChildToHorizontalBox(SquadSizeIncreaseButton);
	AddRow(SizeRow, 8.0f);

	MemberList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MemberList"));
	AddRow(MemberList, 8.0f);

	DeployButton = UOutbreakMenuButton::Create(*WidgetTree, LOCTEXT("Deploy", "  DEPLOY  "), 0);
	AddRow(DeployButton, 24.0f);
}

void UOutbreakBriefingWidget::ShowMission(UOutbreakMissionDefinition* Mission, UOutbreakSquadConfiguration* Configuration)
{
	MissionDefinition = Mission;
	SquadConfiguration = Configuration;
	SizeLimits = OutbreakSquadSelection::ResolveLimits(Configuration);
	Selection = OutbreakSquadSelection::MakeDefault(Configuration, SizeLimits);

	if (MissionNameText)
	{
		MissionNameText->SetText(Mission ? Mission->DisplayName : FText::GetEmpty());
	}
	if (BriefingText)
	{
		BriefingText->SetText(OutbreakMissionText::BuildBriefing(Mission));
	}
	Refresh();
}

void UOutbreakBriefingWidget::ChangeSquadSize(int32 Delta)
{
	OutbreakSquadSelection::SetSquadSize(SquadConfiguration, SizeLimits, Selection, Selection.SquadSize + Delta);
	Refresh();
}

void UOutbreakBriefingWidget::CycleTeammate(int32 Slot, int32 Direction)
{
	OutbreakSquadSelection::CycleTeammate(SquadConfiguration, Selection, Slot, Direction);
	Refresh();
}

void UOutbreakBriefingWidget::CycleLoadout(int32 MemberIndex, int32 Direction)
{
	OutbreakSquadSelection::CycleLoadout(SquadConfiguration, Selection, MemberIndex, Direction);
	Refresh();
}

void UOutbreakBriefingWidget::Deploy()
{
	OnDeployRequested.Broadcast(Selection);
}

void UOutbreakBriefingWidget::Refresh()
{
	if (SquadSizeText)
	{
		SquadSizeText->SetText(FText::Format(LOCTEXT("SquadSize", "Squad size {0}  ({1}-{2}, you included)"),
			Selection.SquadSize, SizeLimits.MinSquadSize, SizeLimits.MaxSquadSize));
	}
	if (SquadSizeDecreaseButton)
	{
		SquadSizeDecreaseButton->SetIsEnabled(Selection.SquadSize > SizeLimits.MinSquadSize);
	}
	if (SquadSizeIncreaseButton)
	{
		SquadSizeIncreaseButton->SetIsEnabled(Selection.SquadSize < SizeLimits.MaxSquadSize);
	}
	RebuildMemberRows();
	OnSelectionChanged();
}

void UOutbreakBriefingWidget::RebuildMemberRows()
{
	if (!MemberList || !WidgetTree)
	{
		return;
	}
	MemberList->ClearChildren();

	const bool bCanChooseLoadouts = SquadConfiguration && !SquadConfiguration->AvailableLoadouts.IsEmpty();
	const bool bCanChooseSoldiers = SquadConfiguration && !SquadConfiguration->Teammates.IsEmpty();

	for (int32 MemberIndex = 0; MemberIndex < Selection.SquadSize; ++MemberIndex)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		auto AddButton = [this, Row](const FText& Label, int32 Action, int32 Index)
		{
			UOutbreakMenuButton* Button = UOutbreakMenuButton::Create(*WidgetTree, Label, Action, Index);
			Button->OnMenuButtonClicked.AddUniqueDynamic(this, &ThisClass::HandleMemberButton);
			Row->AddChildToHorizontalBox(Button);
		};
		auto AddText = [this, Row](const FText& Text)
		{
			UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			TextBlock->SetText(Text);
			Row->AddChildToHorizontalBox(TextBlock)->SetPadding(FMargin(12.0f, 4.0f));
		};

		const bool bIsPlayer = MemberIndex == 0;
		const int32 Slot = MemberIndex - 1;
		const FText Name = bIsPlayer ? LOCTEXT("PlayerRow", "You") : OutbreakMissionText::GetTeammateName(SquadConfiguration, Selection, Slot);
		AddText(FText::Format(LOCTEXT("MemberNumber", "{0}. {1}"), MemberIndex + 1, Name));
		if (!bIsPlayer && bCanChooseSoldiers)
		{
			AddButton(LOCTEXT("PreviousSoldier", " < "), PreviousTeammate, Slot);
			AddButton(LOCTEXT("NextSoldier", " > "), NextTeammate, Slot);
		}

		AddText(OutbreakMissionText::GetLoadoutName(Selection, MemberIndex));
		if (bCanChooseLoadouts)
		{
			AddButton(LOCTEXT("PreviousLoadout", " < "), PreviousLoadout, MemberIndex);
			AddButton(LOCTEXT("NextLoadout", " > "), NextLoadout, MemberIndex);
		}

		MemberList->AddChild(Row);
	}
}

void UOutbreakBriefingWidget::HandleMemberButton(UOutbreakMenuButton* Button)
{
	if (!Button)
	{
		return;
	}
	switch (Button->Action)
	{
	case PreviousTeammate: CycleTeammate(Button->Index, -1); break;
	case NextTeammate:     CycleTeammate(Button->Index, 1); break;
	case PreviousLoadout:  CycleLoadout(Button->Index, -1); break;
	case NextLoadout:      CycleLoadout(Button->Index, 1); break;
	default: break;
	}
}

void UOutbreakBriefingWidget::HandleDecreaseClicked()
{
	ChangeSquadSize(-1);
}

void UOutbreakBriefingWidget::HandleIncreaseClicked()
{
	ChangeSquadSize(1);
}

void UOutbreakBriefingWidget::HandleDeployClicked()
{
	Deploy();
}

#undef LOCTEXT_NAMESPACE
