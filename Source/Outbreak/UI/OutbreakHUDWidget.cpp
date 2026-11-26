#include "UI/OutbreakHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Missions/OutbreakMissionTypes.h"

#define LOCTEXT_NAMESPACE "OutbreakHUD"

UOutbreakHUDWidget::UOutbreakHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	HealthPlaceholderText = LOCTEXT("HealthPlaceholder", "Health --");
	AmmoPlaceholderText = LOCTEXT("AmmoPlaceholder", "-- / --");
}

void UOutbreakHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}

	// The HUD never takes mouse or focus input.
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (HealthText)
	{
		HealthText->SetText(HealthPlaceholderText);
	}
	if (AmmoText)
	{
		AmmoText->SetText(AmmoPlaceholderText);
	}
	SetInteractionPrompt(FText::GetEmpty());
	SetMissionStatus(FOutbreakMissionStatus());
	SetSquadMenu(FText::GetEmpty());
	SetInventory(FText::GetEmpty());
	SetMedicalStatus(FText::GetEmpty());
}

void UOutbreakHUDWidget::BuildDefaultLayout()
{
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	auto AddText = [this, RootCanvas](FName Name, const FText& Text, const FAnchors& Anchors,
		const FVector2D& Alignment, const FVector2D& Position) -> UTextBlock*
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TextBlock->SetText(Text);

		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(TextBlock);
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetAutoSize(true);
		return TextBlock;
	};

	const FAnchors Centre(0.5f, 0.5f);
	const FAnchors BottomLeft(0.0f, 1.0f);
	const FAnchors BottomRight(1.0f, 1.0f);

	Crosshair = AddText(TEXT("Crosshair"), LOCTEXT("Crosshair", "+"), Centre, FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
	InteractionPromptText = AddText(TEXT("InteractionPromptText"), FText::GetEmpty(), Centre, FVector2D(0.5f, 0.0f), FVector2D(0.0f, 40.0f));
	HealthText = AddText(TEXT("HealthText"), HealthPlaceholderText, BottomLeft, FVector2D(0.0f, 1.0f), FVector2D(40.0f, -40.0f));
	AmmoText = AddText(TEXT("AmmoText"), AmmoPlaceholderText, BottomRight, FVector2D(1.0f, 1.0f), FVector2D(-40.0f, -40.0f));
	WeaponText = AddText(TEXT("WeaponText"), FText::GetEmpty(), BottomRight, FVector2D(1.0f, 1.0f), FVector2D(-40.0f, -70.0f));

	const FAnchors TopLeft(0.0f, 0.0f);
	SquadStatusText = AddText(TEXT("SquadStatusText"), FText::GetEmpty(), TopLeft, FVector2D(0.0f, 0.0f), FVector2D(40.0f, 40.0f));

	const FAnchors LeftCentre(0.0f, 0.5f);
	SquadMenuText = AddText(TEXT("SquadMenuText"), FText::GetEmpty(), LeftCentre, FVector2D(0.0f, 0.5f), FVector2D(40.0f, 0.0f));

	const FAnchors TopCentre(0.5f, 0.0f);
	ObjectiveText = AddText(TEXT("ObjectiveText"), FText::GetEmpty(), TopCentre, FVector2D(0.5f, 0.0f), FVector2D(0.0f, 40.0f));
	MissionResultText = AddText(TEXT("MissionResultText"), FText::GetEmpty(), Centre, FVector2D(0.5f, 1.0f), FVector2D(0.0f, -60.0f));

	const FAnchors RightCentre(1.0f, 0.5f);
	InventoryText = AddText(TEXT("InventoryText"), FText::GetEmpty(), RightCentre, FVector2D(1.0f, 0.5f), FVector2D(-40.0f, 0.0f));
	MedicalStatusText = AddText(TEXT("MedicalStatusText"), FText::GetEmpty(), Centre, FVector2D(0.5f, 0.0f), FVector2D(0.0f, 90.0f));
}

void UOutbreakHUDWidget::SetInteractionPrompt(const FText& Prompt)
{
	const bool bPromptVisible = !Prompt.IsEmpty();

	if (InteractionPromptText)
	{
		InteractionPromptText->SetText(Prompt);
		InteractionPromptText->SetVisibility(bPromptVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	OnInteractionPromptChanged(Prompt, bPromptVisible);
}

void UOutbreakHUDWidget::SetHealthDisplay(float CurrentHealth, float MaxHealth)
{
	if (HealthText)
	{
		HealthText->SetText(FText::Format(LOCTEXT("HealthFormat", "Health {0} / {1}"),
			FText::AsNumber(FMath::CeilToInt(CurrentHealth)), FText::AsNumber(FMath::CeilToInt(MaxHealth))));
	}
}

void UOutbreakHUDWidget::SetAmmoDisplay(int32 MagazineAmmo, int32 ReserveAmmo)
{
	if (AmmoText)
	{
		AmmoText->SetText(FText::Format(LOCTEXT("AmmoFormat", "{0} / {1}"),
			FText::AsNumber(MagazineAmmo), FText::AsNumber(ReserveAmmo)));
	}
}

void UOutbreakHUDWidget::ClearAmmoDisplay()
{
	if (AmmoText)
	{
		AmmoText->SetText(AmmoPlaceholderText);
	}
	if (WeaponText)
	{
		WeaponText->SetText(FText::GetEmpty());
	}
}

void UOutbreakHUDWidget::SetWeaponDisplay(const FText& WeaponName, const FText& FireModeName, const FText& AmmoTypeName, bool bReloading)
{
	if (!WeaponText)
	{
		return;
	}

	const FText Status = bReloading ? LOCTEXT("Reloading", "Reloading") : AmmoTypeName;
	WeaponText->SetText(FText::Format(LOCTEXT("WeaponFormat", "{0} | {1} | {2}"), WeaponName, FireModeName, Status));
}

void UOutbreakHUDWidget::SetSquadStatus(const FText& SquadStatus)
{
	if (SquadStatusText)
	{
		SquadStatusText->SetText(SquadStatus);
		SquadStatusText->SetVisibility(SquadStatus.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UOutbreakHUDWidget::SetInventory(const FText& InInventoryText)
{
	if (InventoryText)
	{
		InventoryText->SetText(InInventoryText);
		InventoryText->SetVisibility(InInventoryText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UOutbreakHUDWidget::SetMedicalStatus(const FText& StatusText)
{
	if (MedicalStatusText)
	{
		MedicalStatusText->SetText(StatusText);
		MedicalStatusText->SetVisibility(StatusText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UOutbreakHUDWidget::SetSquadMenu(const FText& MenuText)
{
	if (SquadMenuText)
	{
		SquadMenuText->SetText(MenuText);
		SquadMenuText->SetVisibility(MenuText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UOutbreakHUDWidget::SetMissionStatus(const FOutbreakMissionStatus& Status)
{
	const bool bInProgress = Status.State == EOutbreakMissionState::InProgress;
	const bool bEnded = Status.State == EOutbreakMissionState::Completed || Status.State == EOutbreakMissionState::Failed;

	if (ObjectiveText)
	{
		FText Line = FText::GetEmpty();
		if (bInProgress && !Status.ObjectiveDescription.IsEmpty())
		{
			Line = Status.ObjectiveProgress.IsEmpty()
				? FText::Format(LOCTEXT("ObjectiveLine", "Objective {0}/{1}: {2}"),
					Status.ObjectiveIndex + 1, Status.ObjectiveCount, Status.ObjectiveDescription)
				: FText::Format(LOCTEXT("ObjectiveLineProgress", "Objective {0}/{1}: {2}  ({3})"),
					Status.ObjectiveIndex + 1, Status.ObjectiveCount, Status.ObjectiveDescription, Status.ObjectiveProgress);
		}
		ObjectiveText->SetText(Line);
		ObjectiveText->SetVisibility(Line.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (MissionResultText)
	{
		FText Result = FText::GetEmpty();
		if (Status.State == EOutbreakMissionState::Completed)
		{
			Result = FText::Format(LOCTEXT("MissionComplete", "MISSION COMPLETE\n{0}"), Status.MissionName);
		}
		else if (Status.State == EOutbreakMissionState::Failed)
		{
			Result = FText::Format(LOCTEXT("MissionFailed", "MISSION FAILED\n{0}"), Status.ResultText);
		}
		MissionResultText->SetText(Result);
		MissionResultText->SetVisibility(bEnded ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UOutbreakHUDWidget::HandleInteractionFocusChanged(AActor* FocusedActor, const FText& InteractionPrompt)
{
	SetInteractionPrompt(InteractionPrompt);
}

#undef LOCTEXT_NAMESPACE
