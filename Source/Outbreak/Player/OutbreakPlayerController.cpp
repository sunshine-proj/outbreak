#include "Player/OutbreakPlayerController.h"

#include "AI/OutbreakEnemyAIController.h"
#include "Combat/OutbreakCombatEvents.h"
#include "Combat/OutbreakTeam.h"
#include "GameFramework/Pawn.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Interaction/OutbreakInteractionComponent.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Inventory/OutbreakInventoryText.h"
#include "Engine/World.h"
#include "Game/OutbreakGameMode.h"
#include "Game/OutbreakGameState.h"
#include "Game/OutbreakPlayerState.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Missions/OutbreakMissionText.h"
#include "Net/UnrealNetwork.h"
#include "Outbreak.h"
#include "Player/OutbreakSquadCommandComponent.h"
#include "Squad/OutbreakSquadCommsComponent.h"
#include "Squad/OutbreakSquadSettings.h"
#include "TimerManager.h"
#include "UI/OutbreakBriefingWidget.h"
#include "UI/OutbreakHUDText.h"
#include "UI/OutbreakHUDWidget.h"
#include "UI/OutbreakMissionSummaryWidget.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

AOutbreakPlayerController::AOutbreakPlayerController()
{
	HUDWidgetClass = UOutbreakHUDWidget::StaticClass();
	BriefingWidgetClass = UOutbreakBriefingWidget::StaticClass();
	MissionSummaryWidgetClass = UOutbreakMissionSummaryWidget::StaticClass();
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
		BindToGameState();
	}

	// Server: hit markers for this player's hits, and the stealth awareness meter.
	if (HasAuthority())
	{
		DamageDealtHandle = OutbreakCombatEvents::OnDamageDealt().AddUObject(this, &ThisClass::HandleDamageDealt);
		GetWorldTimerManager().SetTimer(AwarenessTimer, this, &ThisClass::UpdateEnemyAwareness, AwarenessUpdateInterval, true);
	}
}

void AOutbreakPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AOutbreakPlayerController, EnemyAwareness, COND_OwnerOnly);
}

void AOutbreakPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SquadPanelTimer);
	GetWorldTimerManager().ClearTimer(AwarenessTimer);
	GetWorldTimerManager().ClearTimer(HitMarkerTimer);
	OutbreakCombatEvents::OnDamageDealt().Remove(DamageDealtHandle);
	if (UOutbreakSquadCommsComponent* Comms = BoundComms.Get())
	{
		Comms->OnCallout.RemoveDynamic(this, &ThisClass::HandleCallout);
	}
	if (AOutbreakGameState* GameState = BoundGameState.Get())
	{
		GameState->OnMissionStatusChanged.RemoveDynamic(this, &ThisClass::HandleMissionStatusChanged);
	}
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
	// Inventory and medical status (Phase 10): bleed-out and revive timers change continuously.
	if (const APawn* ControlledPawn = GetPawn())
	{
		HUDWidget->SetInventory(OutbreakInventoryText::BuildInventoryText(
			ControlledPawn->FindComponentByClass<UOutbreakInventoryComponent>(), bInventoryPanelOpen));
		HUDWidget->SetMedicalStatus(OutbreakInventoryText::BuildMedicalStatusText(ControlledPawn));
	}

	// The game state replicates the mission status; reading it here also covers late replication.
	BindToGameState();
	const AOutbreakGameState* GameState = GetWorld()->GetGameState<AOutbreakGameState>();
	if (GameState)
	{
		HUDWidget->SetMissionStatus(GameState->GetMissionStatus(), bInventoryPanelOpen);
	}
	RefreshNotifications();

	// Phase 13: stealth meter, alarm, squad subtitles.
	HUDWidget->SetAwareness(OutbreakHUDText::BuildAwarenessText(GetEnemyAwareness(), GameState && GameState->IsAlarmRaised()));
	BindToSquadComms();
	if (SubtitleExpiry > 0.0 && GetWorld()->GetTimeSeconds() >= SubtitleExpiry)
	{
		SubtitleExpiry = 0.0;
		HUDWidget->SetSubtitle(FText::GetEmpty());
	}
}

// --- HUD polish ----------------------------------------------------------------------------------

void AOutbreakPlayerController::UpdateEnemyAwareness()
{
	const float Awareness = AOutbreakEnemyAIController::GetHighestDetectionOf(GetWorld(), GetPawn());
	EnemyAwareness = static_cast<uint8>(FMath::RoundToInt(FMath::Clamp(Awareness, 0.0f, 1.0f) * 100.0f));
}

void AOutbreakPlayerController::HandleDamageDealt(AActor* Victim, AController* DamageInstigator, float Damage, bool bTakenDown)
{
	const APawn* ControlledPawn = GetPawn();
	if (DamageInstigator != this || !Victim || Victim == ControlledPawn)
	{
		return;
	}
	// No marker for hurting friends; enemies and objects (no team) count.
	if (ControlledPawn && OutbreakTeam::GetAttitude(FGenericTeamId::GetTeamIdentifier(ControlledPawn),
		FGenericTeamId::GetTeamIdentifier(Victim)) == ETeamAttitude::Friendly)
	{
		return;
	}
	ClientShowHitMarker(bTakenDown);
}

void AOutbreakPlayerController::ClientShowHitMarker_Implementation(bool bKill)
{
	if (HUDWidget)
	{
		HUDWidget->SetHitMarker(true, bKill);
		GetWorldTimerManager().SetTimer(HitMarkerTimer, this, &ThisClass::HideHitMarker, HitMarkerDuration, false);
	}
}

void AOutbreakPlayerController::HideHitMarker()
{
	if (HUDWidget)
	{
		HUDWidget->SetHitMarker(false, false);
	}
}

void AOutbreakPlayerController::BindToSquadComms()
{
	if (BoundComms.IsValid() || !IsLocalController())
	{
		return;
	}
	const AOutbreakPlayerState* OutbreakPlayerState = GetPlayerState<AOutbreakPlayerState>();
	if (UOutbreakSquadCommsComponent* Comms = OutbreakPlayerState ? OutbreakPlayerState->GetSquadCommsComponent() : nullptr)
	{
		BoundComms = Comms;
		Comms->OnCallout.AddUniqueDynamic(this, &ThisClass::HandleCallout);
	}
}

void AOutbreakPlayerController::HandleCallout(UOutbreakSquadMemberComponent* Speaker, EOutbreakCallout Callout, const FText& Subtitle)
{
	if (!HUDWidget || Subtitle.IsEmpty())
	{
		return;
	}
	HUDWidget->SetSubtitle(Subtitle);
	SubtitleExpiry = GetWorld()->GetTimeSeconds() + GetDefault<UOutbreakSquadSettings>()->Comms.SubtitleDuration;
}

// --- Mission flow --------------------------------------------------------------------------------

void AOutbreakPlayerController::BindToGameState()
{
	if (!IsLocalController() || BoundGameState.IsValid())
	{
		return;
	}
	AOutbreakGameState* GameState = GetWorld()->GetGameState<AOutbreakGameState>();
	if (!GameState)
	{
		// Not replicated yet on a client; the refresh timer tries again.
		return;
	}
	BoundGameState = GameState;
	GameState->OnMissionStatusChanged.AddUniqueDynamic(this, &ThisClass::HandleMissionStatusChanged);

	// No notifications for what happened before this controller existed; just show the right screen.
	LastMissionStatus = GameState->GetMissionStatus();
	UpdateMissionScreens(LastMissionStatus);
}

void AOutbreakPlayerController::HandleMissionStatusChanged(const FOutbreakMissionStatus& Status)
{
	const double Now = GetWorld()->GetTimeSeconds();
	for (const FText& Notification : OutbreakMissionText::BuildObjectiveNotifications(LastMissionStatus, Status))
	{
		Notifications.Emplace(Notification, Now + NotificationDuration);
	}
	while (Notifications.Num() > FMath::Max(1, MaxNotifications))
	{
		Notifications.RemoveAt(0);
	}
	LastMissionStatus = Status;

	if (HUDWidget)
	{
		HUDWidget->SetMissionStatus(Status, bInventoryPanelOpen);
	}
	RefreshNotifications();
	UpdateMissionScreens(Status);
}

void AOutbreakPlayerController::RefreshNotifications()
{
	const double Now = GetWorld()->GetTimeSeconds();
	Notifications.RemoveAll([Now](const TPair<FText, double>& Entry) { return Entry.Value <= Now; });
	if (HUDWidget)
	{
		TArray<FText> Lines;
		for (const TPair<FText, double>& Entry : Notifications)
		{
			Lines.Add(Entry.Key);
		}
		HUDWidget->SetNotifications(FText::Join(FText::FromString(TEXT("\n")), Lines));
	}
}

void AOutbreakPlayerController::UpdateMissionScreens(const FOutbreakMissionStatus& Status)
{
	if (!IsLocalController())
	{
		return;
	}
	if (Status.State == EOutbreakMissionState::Briefing && !bDeployRequested)
	{
		ShowBriefing();
	}
	else
	{
		HideBriefing();
	}
	if (Status.State == EOutbreakMissionState::Completed || Status.State == EOutbreakMissionState::Failed)
	{
		ShowMissionSummary();
	}
	ApplyInputMode();
}

void AOutbreakPlayerController::ShowBriefing()
{
	const AOutbreakGameState* GameState = BoundGameState.Get();
	if (BriefingWidget || !BriefingWidgetClass || !GameState)
	{
		return;
	}
	BriefingWidget = CreateWidget<UOutbreakBriefingWidget>(this, BriefingWidgetClass);
	if (!BriefingWidget)
	{
		return;
	}
	BriefingWidget->OnDeployRequested.AddUniqueDynamic(this, &ThisClass::HandleBriefingDeployRequested);
	BriefingWidget->AddToViewport(10);
	BriefingWidget->ShowMission(GameState->GetMissionDefinition(), GameState->GetSquadConfiguration());
}

void AOutbreakPlayerController::HideBriefing()
{
	if (BriefingWidget)
	{
		BriefingWidget->RemoveFromParent();
		BriefingWidget = nullptr;
	}
}

void AOutbreakPlayerController::ShowMissionSummary()
{
	const AOutbreakGameState* GameState = BoundGameState.Get();
	if (MissionSummaryWidget || !MissionSummaryWidgetClass || !GameState)
	{
		return;
	}
	MissionSummaryWidget = CreateWidget<UOutbreakMissionSummaryWidget>(this, MissionSummaryWidgetClass);
	if (!MissionSummaryWidget)
	{
		return;
	}
	MissionSummaryWidget->OnRetryRequested.AddUniqueDynamic(this, &ThisClass::HandleSummaryRetry);
	MissionSummaryWidget->OnQuitRequested.AddUniqueDynamic(this, &ThisClass::HandleSummaryQuit);
	MissionSummaryWidget->AddToViewport(20);
	MissionSummaryWidget->ShowResults(GameState->GetMissionResults());
}

void AOutbreakPlayerController::ApplyInputMode()
{
	UUserWidget* Screen = MissionSummaryWidget.Get();
	if (!Screen)
	{
		Screen = BriefingWidget.Get();
	}
	const bool bMenu = Screen != nullptr;
	if (bMenu == bShowMouseCursor)
	{
		return;
	}

	if (bMenu)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(Screen->TakeWidget());
		SetInputMode(InputMode);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
	bShowMouseCursor = bMenu;

	// The screens cover the HUD; hiding it avoids two result texts on top of each other.
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(bMenu ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void AOutbreakPlayerController::RequestDeploy(const FOutbreakDeploymentSelection& Selection)
{
	if (bDeployRequested)
	{
		return;
	}
	bDeployRequested = true;
	HideBriefing();
	ApplyInputMode();
	ServerRequestDeploy(Selection);
}

void AOutbreakPlayerController::ServerRequestDeploy_Implementation(const FOutbreakDeploymentSelection& Selection)
{
	if (AOutbreakGameMode* GameMode = GetWorld()->GetAuthGameMode<AOutbreakGameMode>())
	{
		GameMode->HandleDeployRequest(this, Selection);
	}
}

void AOutbreakPlayerController::RequestRestartMission()
{
	ServerRequestRestartMission();
}

void AOutbreakPlayerController::ServerRequestRestartMission_Implementation()
{
	if (AOutbreakGameMode* GameMode = GetWorld()->GetAuthGameMode<AOutbreakGameMode>())
	{
		GameMode->HandleRestartRequest(this);
	}
}

void AOutbreakPlayerController::QuitMission()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/ false);
}

void AOutbreakPlayerController::HandleBriefingDeployRequested(const FOutbreakDeploymentSelection& Selection)
{
	RequestDeploy(Selection);
}

void AOutbreakPlayerController::HandleSummaryRetry()
{
	RequestRestartMission();
}

void AOutbreakPlayerController::HandleSummaryQuit()
{
	QuitMission();
}

void AOutbreakPlayerController::ToggleInventoryPanel()
{
	bInventoryPanelOpen = !bInventoryPanelOpen;
	RefreshSquadPanel();
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
