#include "UI/OutbreakCampaignMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Campaign/OutbreakCampaignDefinition.h"
#include "Campaign/OutbreakCampaignSubsystem.h"
#include "Campaign/OutbreakCampaignText.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Game/OutbreakGameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "UI/OutbreakMenuButton.h"

#define LOCTEXT_NAMESPACE "OutbreakCampaignMenu"

namespace
{
	constexpr float SensitivityStep = 0.1f;
	constexpr float FieldOfViewStep = 5.0f;
	/** Shown when the field-of-view setting is 0 (character default) and the player starts changing it. */
	constexpr float DefaultFieldOfViewForSteps = 90.0f;
}

void UOutbreakCampaignMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleContinueClicked);
	}
	if (NewCampaignButton)
	{
		NewCampaignButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleNewCampaignClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleQuitClicked);
	}
	if (UOutbreakCampaignSubsystem* Campaign = GetCampaign())
	{
		Campaign->OnCampaignChanged.AddUniqueDynamic(this, &ThisClass::HandleCampaignChanged);
	}
	Refresh();
}

void UOutbreakCampaignMenuWidget::NativeDestruct()
{
	if (UOutbreakCampaignSubsystem* Campaign = GetCampaign())
	{
		Campaign->OnCampaignChanged.RemoveDynamic(this, &ThisClass::HandleCampaignChanged);
	}
	Super::NativeDestruct();
}

UOutbreakCampaignSubsystem* UOutbreakCampaignMenuWidget::GetCampaign() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UOutbreakCampaignSubsystem>() : nullptr;
}

UTextBlock* UOutbreakCampaignMenuWidget::MakeText(const FText& Text) const
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(Text);
	TextBlock->SetAutoWrapText(true);
	return TextBlock;
}

UOutbreakMenuButton* UOutbreakCampaignMenuWidget::MakeButton(const FText& Label, EAction Action, int32 Index)
{
	UOutbreakMenuButton* Button = UOutbreakMenuButton::Create(*WidgetTree, Label, static_cast<int32>(Action), Index);
	Button->OnMenuButtonClicked.AddUniqueDynamic(this, &ThisClass::HandleRowButton);
	return Button;
}

void UOutbreakCampaignMenuWidget::BuildDefaultLayout()
{
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.04f, 1.0f));
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

	TitleText = MakeText(FText::GetEmpty());
	AddRow(TitleText, 0.0f);
	BriefingText = MakeText(FText::GetEmpty());
	AddRow(BriefingText, 16.0f);
	ProgressText = MakeText(FText::GetEmpty());
	AddRow(ProgressText, 12.0f);

	UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Actions"));
	ContinueButton = UOutbreakMenuButton::Create(*WidgetTree, LOCTEXT("Continue", "  CONTINUE  "), 0);
	NewCampaignButton = UOutbreakMenuButton::Create(*WidgetTree, LOCTEXT("NewCampaign", "  NEW CAMPAIGN  "), 0);
	QuitButton = UOutbreakMenuButton::Create(*WidgetTree, LOCTEXT("Quit", "  QUIT  "), 0);
	Actions->AddChildToHorizontalBox(ContinueButton);
	Actions->AddChildToHorizontalBox(NewCampaignButton)->SetPadding(FMargin(16.0f, 0.0f, 0.0f, 0.0f));
	Actions->AddChildToHorizontalBox(QuitButton)->SetPadding(FMargin(16.0f, 0.0f, 0.0f, 0.0f));
	AddRow(Actions, 20.0f);

	MessageText = MakeText(FText::GetEmpty());
	AddRow(MessageText, 8.0f);

	AddRow(MakeText(LOCTEXT("MissionsHeader", "MISSIONS")), 28.0f);
	MissionList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MissionList"));
	AddRow(MissionList, 8.0f);

	AddRow(MakeText(LOCTEXT("SavesHeader", "SAVED GAMES")), 28.0f);
	SlotList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SlotList"));
	AddRow(SlotList, 8.0f);

	AddRow(MakeText(LOCTEXT("SettingsHeader", "SETTINGS")), 28.0f);
	SettingsList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsList"));
	AddRow(SettingsList, 8.0f);
}

void UOutbreakCampaignMenuWidget::Refresh()
{
	const UOutbreakCampaignSubsystem* Campaign = GetCampaign();
	const UOutbreakCampaignDefinition* Definition = Campaign ? Campaign->GetCampaign() : nullptr;

	if (TitleText)
	{
		TitleText->SetText(Definition
			? (Definition->DisplayName.IsEmpty() ? FText::FromName(Definition->GetResolvedCampaignId()) : Definition->DisplayName)
			: LOCTEXT("NoCampaign", "No campaign is set up (Project Settings > Game > Outbreak Campaign)."));
	}
	if (BriefingText)
	{
		const bool bComplete = Campaign && Campaign->IsCampaignComplete();
		BriefingText->SetText(Definition
			? (bComplete && !Definition->CompletionText.IsEmpty() ? Definition->CompletionText : Definition->Briefing)
			: FText::GetEmpty());
	}
	if (ProgressText)
	{
		FText Progress;
		if (Definition && Campaign->IsCampaignActive())
		{
			int32 Completed = 0;
			int32 Total = 0;
			OutbreakCampaignRules::CountRequired(*Definition, Campaign->GetProgress(), Completed, Total);
			Progress = OutbreakCampaignText::BuildProgressLine(Completed, Total, Campaign->GetProgress().TotalMissionTime);
		}
		else if (Definition)
		{
			Progress = LOCTEXT("NotStarted", "Start a new campaign or load a saved game.");
		}
		ProgressText->SetText(Progress);
	}

	// Continue: the active campaign's checkpoint or next mission, else the autosave.
	const bool bCanContinueActive = Campaign && Campaign->IsCampaignActive() && (Campaign->HasCheckpoint() || !Campaign->GetNextMissionId().IsNone());
	const bool bAutosaveUsable = Campaign && !Campaign->IsCampaignActive() && Campaign->GetSlotSummary(0).bExists && !Campaign->GetSlotSummary(0).bUnreadable;
	if (ContinueButton)
	{
		ContinueButton->SetIsEnabled(Definition && (bCanContinueActive || bAutosaveUsable));
	}
	if (NewCampaignButton)
	{
		NewCampaignButton->SetIsEnabled(Definition != nullptr);
		if (UOutbreakMenuButton* Button = Cast<UOutbreakMenuButton>(NewCampaignButton))
		{
			Button->SetLabel(bConfirmNewCampaign
				? LOCTEXT("ConfirmNew", "  CONFIRM: START OVER (REPLACES THE AUTOSAVE)  ")
				: LOCTEXT("NewCampaign", "  NEW CAMPAIGN  "));
		}
	}
	if (MessageText)
	{
		MessageText->SetText(LastMessage);
	}

	RebuildMissionRows();
	RebuildSlotRows();
	RebuildSettingsRows();
	OnMenuRefreshed();
}

void UOutbreakCampaignMenuWidget::RebuildMissionRows()
{
	if (!MissionList)
	{
		return;
	}
	MissionList->ClearChildren();

	const UOutbreakCampaignSubsystem* Campaign = GetCampaign();
	const UOutbreakCampaignDefinition* Definition = Campaign ? Campaign->GetCampaign() : nullptr;
	if (!Definition)
	{
		return;
	}
	const bool bActive = Campaign->IsCampaignActive();
	const FOutbreakCampaignProgress& Progress = Campaign->GetProgress();

	for (int32 Index = 0; Index < Definition->Missions.Num(); ++Index)
	{
		const FOutbreakCampaignMission& Entry = Definition->Missions[Index];
		const EOutbreakCampaignMissionStatus Status = bActive
			? OutbreakCampaignRules::GetMissionStatus(*Definition, Progress, Index) : EOutbreakCampaignMissionStatus::Locked;
		const bool bHasCheckpoint = bActive && Progress.Checkpoint.MissionId == Entry.MissionId;

		TArray<FText> RequiredNames;
		if (Status == EOutbreakCampaignMissionStatus::Locked && bActive)
		{
			if (Entry.Unlock == EOutbreakMissionUnlock::AfterMissions)
			{
				for (const FName Required : Entry.RequiredMissions)
				{
					RequiredNames.Add(Campaign->GetMissionDisplayName(Required));
				}
			}
			else if (Entry.Unlock == EOutbreakMissionUnlock::AfterPrevious)
			{
				for (int32 Previous = Index - 1; Previous >= 0; --Previous)
				{
					if (!Definition->Missions[Previous].bOptional)
					{
						RequiredNames.Add(Campaign->GetMissionDisplayName(Definition->Missions[Previous].MissionId));
						break;
					}
				}
			}
		}

		UVerticalBox* Row = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* Line = MakeText(OutbreakCampaignText::BuildMissionLine(Index, Campaign->GetMissionDisplayName(Entry.MissionId), Status,
			Progress.FindRecord(Entry.MissionId), bHasCheckpoint, RequiredNames, Entry.bOptional));
		Header->AddChildToHorizontalBox(Line)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		if (Status != EOutbreakCampaignMissionStatus::Locked)
		{
			const FText Label = Status == EOutbreakCampaignMissionStatus::Completed ? LOCTEXT("Replay", "  REPLAY  ")
				: (bHasCheckpoint ? LOCTEXT("Restart", "  RESTART  ") : LOCTEXT("Launch", "  LAUNCH  "));
			Header->AddChildToHorizontalBox(MakeButton(Label, EAction::Launch, Index))->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
		}
		Row->AddChildToVerticalBox(Header);

		// The mission's own description, for missions the player can see the way to.
		if (Status != EOutbreakCampaignMissionStatus::Locked)
		{
			if (const UOutbreakMissionDefinition* Mission = Entry.Mission.LoadSynchronous())
			{
				if (!Mission->Description.IsEmpty())
				{
					Row->AddChildToVerticalBox(MakeText(Mission->Description))->SetPadding(FMargin(24.0f, 2.0f, 0.0f, 0.0f));
				}
			}
		}
		MissionList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 4.0f));
	}
}

void UOutbreakCampaignMenuWidget::RebuildSlotRows()
{
	if (!SlotList)
	{
		return;
	}
	SlotList->ClearChildren();

	const UOutbreakCampaignSubsystem* Campaign = GetCampaign();
	if (!Campaign || !Campaign->GetCampaign())
	{
		return;
	}
	for (int32 SlotIndex = 0; SlotIndex < Campaign->GetSlotCount(); ++SlotIndex)
	{
		const FOutbreakSaveSlotSummary Summary = Campaign->GetSlotSummary(SlotIndex);
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Row->AddChildToHorizontalBox(MakeText(OutbreakCampaignText::BuildSlotLine(Summary)))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		if (Summary.bExists && !Summary.bUnreadable)
		{
			Row->AddChildToHorizontalBox(MakeButton(LOCTEXT("Load", "  LOAD  "), EAction::Load, SlotIndex))->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
		}
		// The autosave is written by the game only.
		if (SlotIndex > 0 && Campaign->IsCampaignActive())
		{
			Row->AddChildToHorizontalBox(MakeButton(LOCTEXT("Save", "  SAVE  "), EAction::Save, SlotIndex))->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
		}
		if (Summary.bExists)
		{
			const FText Label = ConfirmDeleteSlot == SlotIndex ? LOCTEXT("ConfirmDelete", "  CONFIRM DELETE  ") : LOCTEXT("Delete", "  DELETE  ");
			Row->AddChildToHorizontalBox(MakeButton(Label, EAction::Delete, SlotIndex))->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
		}
		SlotList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 4.0f));
	}
}

void UOutbreakCampaignMenuWidget::RebuildSettingsRows()
{
	if (!SettingsList)
	{
		return;
	}
	SettingsList->ClearChildren();

	const UOutbreakGameUserSettings* Settings = UOutbreakGameUserSettings::GetOutbreakUserSettings();
	if (!Settings)
	{
		SettingsList->AddChildToVerticalBox(MakeText(LOCTEXT("NoSettings", "Settings unavailable (see the log).")));
		return;
	}

	auto AddStepper = [this](const FText& Label, const FText& Value, EAction Down, EAction Up)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Row->AddChildToHorizontalBox(MakeText(Label))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Row->AddChildToHorizontalBox(MakeButton(LOCTEXT("Down", " - "), Down, 0));
		Row->AddChildToHorizontalBox(MakeText(Value))->SetPadding(FMargin(12.0f, 4.0f));
		if (Up != Down)
		{
			Row->AddChildToHorizontalBox(MakeButton(LOCTEXT("Up", " + "), Up, 0));
		}
		SettingsList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 2.0f));
	};
	auto AddToggle = [this](const FText& Label, bool bOn, EAction Action)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Row->AddChildToHorizontalBox(MakeText(Label))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Row->AddChildToHorizontalBox(MakeButton(bOn ? LOCTEXT("On", "  ON  ") : LOCTEXT("Off", "  OFF  "), Action, 0));
		SettingsList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 2.0f));
	};

	FNumberFormattingOptions OneDecimal;
	OneDecimal.MinimumFractionalDigits = 1;
	OneDecimal.MaximumFractionalDigits = 1;
	AddStepper(LOCTEXT("Sensitivity", "Look sensitivity"), FText::AsNumber(Settings->GetLookSensitivity(), &OneDecimal),
		EAction::SensitivityDown, EAction::SensitivityUp);
	AddToggle(LOCTEXT("Invert", "Invert look up/down"), Settings->IsLookYInverted(), EAction::ToggleInvert);
	AddStepper(LOCTEXT("FieldOfView", "Field of view"),
		Settings->GetFieldOfView() > 0.0f ? FText::AsNumber(FMath::RoundToInt(Settings->GetFieldOfView())) : LOCTEXT("FovDefault", "Default"),
		EAction::FieldOfViewDown, EAction::FieldOfViewUp);
	AddToggle(LOCTEXT("Subtitles", "Subtitles"), Settings->AreSubtitlesShown(), EAction::ToggleSubtitles);

	UOutbreakMenuButton* Apply = MakeButton(LOCTEXT("Apply", "  APPLY AND SAVE SETTINGS  "), EAction::ApplySettings, 0);
	SettingsList->AddChildToVerticalBox(Apply)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
}

void UOutbreakCampaignMenuWidget::SetMessage(const FText& Message)
{
	LastMessage = Message;
	if (MessageText)
	{
		MessageText->SetText(LastMessage);
	}
}

void UOutbreakCampaignMenuWidget::HandleCampaignChanged()
{
	Refresh();
}

void UOutbreakCampaignMenuWidget::HandleContinueClicked()
{
	UOutbreakCampaignSubsystem* Campaign = GetCampaign();
	if (!Campaign)
	{
		return;
	}
	if (!Campaign->IsCampaignActive() && !Campaign->LoadFromSlot(0))
	{
		SetMessage(Campaign->GetLastSaveMessage());
		return;
	}
	if (!Campaign->ContinueCampaign())
	{
		SetMessage(Campaign->IsCampaignComplete()
			? LOCTEXT("AllDone", "The campaign is complete. Replay any mission from the list.")
			: LOCTEXT("CannotContinue", "No mission is available to continue."));
	}
}

void UOutbreakCampaignMenuWidget::HandleNewCampaignClicked()
{
	UOutbreakCampaignSubsystem* Campaign = GetCampaign();
	if (!Campaign)
	{
		return;
	}
	// Starting over replaces the autosave, so existing progress needs a second click.
	const bool bHasProgress = Campaign->IsCampaignActive() || Campaign->GetSlotSummary(0).bExists;
	if (bHasProgress && !bConfirmNewCampaign)
	{
		bConfirmNewCampaign = true;
		Refresh();
		return;
	}
	bConfirmNewCampaign = false;
	SetMessage(Campaign->StartNewCampaign() ? LOCTEXT("Started", "New campaign started.") : LOCTEXT("StartFailed", "The campaign could not be started (see the log)."));
	Refresh();
}

void UOutbreakCampaignMenuWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/ false);
}

void UOutbreakCampaignMenuWidget::HandleRowButton(UOutbreakMenuButton* Button)
{
	UOutbreakCampaignSubsystem* Campaign = GetCampaign();
	UOutbreakGameUserSettings* Settings = UOutbreakGameUserSettings::GetOutbreakUserSettings();
	if (!Button || !Campaign)
	{
		return;
	}
	const UOutbreakCampaignDefinition* Definition = Campaign->GetCampaign();
	const EAction Action = static_cast<EAction>(Button->Action);
	if (Action != EAction::Delete)
	{
		ConfirmDeleteSlot = INDEX_NONE;
	}
	bConfirmNewCampaign = false;

	switch (Action)
	{
	case EAction::Launch:
		if (Definition && Definition->Missions.IsValidIndex(Button->Index) && !Campaign->LaunchMission(Definition->Missions[Button->Index].MissionId))
		{
			SetMessage(LOCTEXT("LaunchFailed", "The mission could not be launched (see the log)."));
		}
		return;

	case EAction::Load:
		SetMessage(Campaign->LoadFromSlot(Button->Index)
			? FText::Format(LOCTEXT("Loaded", "Loaded {0}."), Button->Index == 0 ? LOCTEXT("TheAutosave", "the autosave") : FText::Format(LOCTEXT("SlotN", "slot {0}"), Button->Index))
			: Campaign->GetLastSaveMessage());
		break;

	case EAction::Save:
		Campaign->SaveToSlot(Button->Index);
		SetMessage(Campaign->GetLastSaveMessage());
		break;

	case EAction::Delete:
		if (ConfirmDeleteSlot != Button->Index)
		{
			ConfirmDeleteSlot = Button->Index;
			break;
		}
		ConfirmDeleteSlot = INDEX_NONE;
		SetMessage(Campaign->DeleteSlot(Button->Index) ? LOCTEXT("Deleted", "Save deleted.") : LOCTEXT("DeleteFailed", "The save could not be deleted."));
		break;

	case EAction::SensitivityDown:
	case EAction::SensitivityUp:
		if (Settings)
		{
			const float Step = Action == EAction::SensitivityUp ? SensitivityStep : -SensitivityStep;
			Settings->SetLookSensitivity(Settings->GetLookSensitivity() + Step);
		}
		break;

	case EAction::ToggleInvert:
		if (Settings)
		{
			Settings->SetInvertLookY(!Settings->IsLookYInverted());
		}
		break;

	case EAction::FieldOfViewDown:
	case EAction::FieldOfViewUp:
		if (Settings)
		{
			const float Current = Settings->GetFieldOfView() > 0.0f ? Settings->GetFieldOfView() : DefaultFieldOfViewForSteps;
			Settings->SetFieldOfView(Current + (Action == EAction::FieldOfViewUp ? FieldOfViewStep : -FieldOfViewStep));
		}
		break;

	case EAction::ToggleSubtitles:
		if (Settings)
		{
			Settings->SetShowSubtitles(!Settings->AreSubtitlesShown());
		}
		break;

	case EAction::ApplySettings:
		if (Settings)
		{
			Settings->ApplySettings(/*bCheckForCommandLineOverrides=*/ false);
			SetMessage(LOCTEXT("SettingsSaved", "Settings saved."));
		}
		break;
	}
	Refresh();
}

#undef LOCTEXT_NAMESPACE
