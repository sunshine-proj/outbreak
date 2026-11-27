#include "Player/OutbreakFrontEndPlayerController.h"

#include "Game/OutbreakGameUserSettings.h"
#include "UI/OutbreakCampaignMenuWidget.h"

AOutbreakFrontEndPlayerController::AOutbreakFrontEndPlayerController()
{
	CampaignMenuWidgetClass = UOutbreakCampaignMenuWidget::StaticClass();
}

void AOutbreakFrontEndPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}
	HandleUserSettingsChanged();
	UOutbreakGameUserSettings::OnGameplaySettingsChanged.AddUObject(this, &ThisClass::HandleUserSettingsChanged);
	if (!CampaignMenuWidgetClass)
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

void AOutbreakFrontEndPlayerController::HandleUserSettingsChanged()
{
	if (const UOutbreakGameUserSettings* Settings = UOutbreakGameUserSettings::GetOutbreakUserSettings())
	{
		Settings->ApplyAudioVolumes(this);
	}
}
