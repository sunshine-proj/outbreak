#include "Player/OutbreakPlayerController.h"

#include "GameFramework/Pawn.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Interaction/OutbreakInteractionComponent.h"
#include "Engine/World.h"
#include "Game/OutbreakGameState.h"
#include "Outbreak.h"
#include "Player/OutbreakSquadCommandComponent.h"
#include "TimerManager.h"
#include "UI/OutbreakHUDWidget.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

AOutbreakPlayerController::AOutbreakPlayerController()
{
	HUDWidgetClass = UOutbreakHUDWidget::StaticClass();
	SquadCommandComponent = CreateDefaultSubobject<UOutbreakSquadCommandComponent>(TEXT("SquadCommandComponent"));
}

void AOutbreakPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Possession can happen before or after BeginPlay depending on how the player was spawned.
	CreateHUDWidget();
	BindHUDToPawn(GetPawn());

	if (IsLocalController())
	{
		GetWorldTimerManager().SetTimer(SquadPanelTimer, this, &ThisClass::RefreshSquadPanel, SquadPanelRefreshInterval, true);
	}
}

void AOutbreakPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SquadPanelTimer);
	Super::EndPlay(EndPlayReason);
}

void AOutbreakPlayerController::RefreshSquadPanel()
{
	if (!HUDWidget)
	{
		return;
	}
	if (SquadCommandComponent)
	{
		HUDWidget->SetSquadStatus(SquadCommandComponent->BuildSquadStatusText());
		HUDWidget->SetSquadMenu(SquadCommandComponent->BuildMenuText());
	}
	// The game state replicates the mission status; reading it here also covers late replication.
	if (const AOutbreakGameState* GameState = GetWorld()->GetGameState<AOutbreakGameState>())
	{
		HUDWidget->SetMissionStatus(GameState->GetMissionStatus());
	}
}

void AOutbreakPlayerController::SetPawn(APawn* InPawn)
{
	APawn* OldPawn = GetPawn();
	if (OldPawn == InPawn)
	{
		Super::SetPawn(InPawn);
		return;
	}

	UnbindHUDFromPawn(OldPawn);
	Super::SetPawn(InPawn);

	if (InPawn)
	{
		CreateHUDWidget();
		BindHUDToPawn(InPawn);
	}
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

	if (UOutbreakHealthComponent* Health = InPawn->FindComponentByClass<UOutbreakHealthComponent>())
	{
		Health->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::HandlePawnHealthChanged);
		HandlePawnHealthChanged(Health, Health->GetHealth(), 0.0f);
	}

	if (UOutbreakWeaponComponent* Weapons = InPawn->FindComponentByClass<UOutbreakWeaponComponent>())
	{
		Weapons->OnWeaponStateChanged.AddUniqueDynamic(this, &ThisClass::HandlePawnWeaponStateChanged);
		HandlePawnWeaponStateChanged(Weapons);
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
	if (UOutbreakHealthComponent* Health = InPawn->FindComponentByClass<UOutbreakHealthComponent>())
	{
		Health->OnHealthChanged.RemoveDynamic(this, &ThisClass::HandlePawnHealthChanged);
	}
	if (UOutbreakWeaponComponent* Weapons = InPawn->FindComponentByClass<UOutbreakWeaponComponent>())
	{
		Weapons->OnWeaponStateChanged.RemoveDynamic(this, &ThisClass::HandlePawnWeaponStateChanged);
	}
	HUDWidget->ClearAmmoDisplay();

	HUDWidget->SetInteractionPrompt(FText::GetEmpty());
}

void AOutbreakPlayerController::HandlePawnHealthChanged(UOutbreakHealthComponent* HealthComponent, float NewHealth, float Delta)
{
	if (HUDWidget && HealthComponent)
	{
		HUDWidget->SetHealthDisplay(NewHealth, HealthComponent->GetMaxHealth());
	}
}

void AOutbreakPlayerController::HandlePawnWeaponStateChanged(UOutbreakWeaponComponent* WeaponComponent)
{
	if (!HUDWidget || !WeaponComponent)
	{
		return;
	}

	FOutbreakWeaponSlot Slot;
	if (!WeaponComponent->GetActiveWeaponSlot(Slot) || !Slot.Definition)
	{
		HUDWidget->ClearAmmoDisplay();
		return;
	}

	const FOutbreakFireModeDefinition* FireMode = WeaponComponent->GetActiveFireMode();
	// The HUD shows the ammunition the next reload will use, so a pending type switch is visible.
	const UOutbreakAmmoTypeDefinition* ShownAmmo = Slot.SelectedAmmoType ? Slot.SelectedAmmoType.Get() : Slot.LoadedAmmoType.Get();

	HUDWidget->SetAmmoDisplay(Slot.AmmoInMagazine, WeaponComponent->GetReserveAmmo(ShownAmmo));
	HUDWidget->SetWeaponDisplay(
		Slot.Definition->DisplayName,
		FireMode ? FireMode->DisplayName : FText::GetEmpty(),
		ShownAmmo ? ShownAmmo->DisplayName : FText::GetEmpty(),
		WeaponComponent->IsReloading());
}
