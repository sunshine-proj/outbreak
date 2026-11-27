#include "Player/OutbreakFrontEndPlayerController.h"

#include "UI/OutbreakCampaignMenuWidget.h"

AOutbreakFrontEndPlayerController::AOutbreakFrontEndPlayerController()
{
	CampaignMenuWidgetClass = UOutbreakCampaignMenuWidget::StaticClass();
}

void AOutbreakFrontEndPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController() || !CampaignMenuWidgetClass)
	{
		return;
	}
	CampaignMenuWidget = CreateWidget<UOutbreakCampaignMenuWidget>(this, CampaignMenuWidgetClass);
	if (!CampaignMenuWidget)
	{
		return;
	}
	CampaignMenuWidget->AddToViewport();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(CampaignMenuWidget->TakeWidget());
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}
