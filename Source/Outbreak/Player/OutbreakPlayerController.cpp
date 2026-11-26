#include "Player/OutbreakPlayerController.h"

#include "GameFramework/Pawn.h"
#include "Interaction/OutbreakInteractionComponent.h"
#include "Outbreak.h"
#include "UI/OutbreakHUDWidget.h"

AOutbreakPlayerController::AOutbreakPlayerController()
{
	HUDWidgetClass = UOutbreakHUDWidget::StaticClass();
}

void AOutbreakPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Possession can happen before or after BeginPlay depending on how the player was spawned.
	CreateHUDWidget();
	BindHUDToPawn(GetPawn());
}

void AOutbreakPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	CreateHUDWidget();
	BindHUDToPawn(InPawn);
}

void AOutbreakPlayerController::OnUnPossess()
{
	UnbindHUDFromPawn(GetPawn());

	Super::OnUnPossess();
}

void AOutbreakPlayerController::CreateHUDWidget()
{
	if (HUDWidget || !IsLocalController())
	{
		return;
	}

	if (!HUDWidgetClass)
	{
		UE_LOG(LogOutbreak, Warning, TEXT("%s: HUDWidgetClass is not set; no HUD will be shown."), *GetName());
		return;
	}

	HUDWidget = CreateWidget<UOutbreakHUDWidget>(this, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
	}
}

void AOutbreakPlayerController::BindHUDToPawn(APawn* InPawn)
{
	if (!HUDWidget || !InPawn)
	{
		return;
	}

	if (UOutbreakInteractionComponent* Interaction = InPawn->FindComponentByClass<UOutbreakInteractionComponent>())
	{
		Interaction->OnFocusChanged.AddUniqueDynamic(HUDWidget.Get(), &UOutbreakHUDWidget::HandleInteractionFocusChanged);
		HUDWidget->SetInteractionPrompt(Interaction->GetFocusedPrompt());
	}
}

void AOutbreakPlayerController::UnbindHUDFromPawn(APawn* InPawn)
{
	if (!HUDWidget || !InPawn)
	{
		return;
	}

	if (UOutbreakInteractionComponent* Interaction = InPawn->FindComponentByClass<UOutbreakInteractionComponent>())
	{
		Interaction->OnFocusChanged.RemoveDynamic(HUDWidget.Get(), &UOutbreakHUDWidget::HandleInteractionFocusChanged);
	}

	HUDWidget->SetInteractionPrompt(FText::GetEmpty());
}
