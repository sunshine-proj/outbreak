#include "UI/OutbreakHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

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

void UOutbreakHUDWidget::HandleInteractionFocusChanged(AActor* FocusedActor, const FText& InteractionPrompt)
{
	SetInteractionPrompt(InteractionPrompt);
}

#undef LOCTEXT_NAMESPACE
