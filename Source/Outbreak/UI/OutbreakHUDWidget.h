#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "OutbreakHUDWidget.generated.h"

class UTextBlock;
class UWidget;
struct FOutbreakMissionStatus;

/**
 * Main in-game HUD: crosshair, health, ammunition, current weapon and interaction prompt.
 *
 * Designers subclass it as a Widget Blueprint and add widgets with the bound names below
 * (all optional). When used directly from C++ with no designer layout, it builds a minimal
 * default layout so the HUD works before any UI assets exist.
 *
 * The widget only displays what it is given. AOutbreakPlayerController pushes values from the
 * pawn's health and weapon components. Placeholders show until the first values arrive.
 */
UCLASS(Blueprintable)
class OUTBREAK_API UOutbreakHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UOutbreakHUDWidget(const FObjectInitializer& ObjectInitializer);

	/** Shows Prompt, or hides the prompt when it is empty. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetInteractionPrompt(const FText& Prompt);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetHealthDisplay(float CurrentHealth, float MaxHealth);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetAmmoDisplay(int32 MagazineAmmo, int32 ReserveAmmo);

	/** Shows the placeholder ammunition text, for example when no weapon is equipped. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ClearAmmoDisplay();

	/** Weapon line, for example "Rifle | Automatic | Standard" or "... | Reloading". */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetWeaponDisplay(const FText& WeaponName, const FText& FireModeName, const FText& AmmoTypeName, bool bReloading);

	/** Multi-line squad panel (members, orders, status). Empty hides it. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetSquadStatus(const FText& SquadStatus);

	/** Squad command menu. Empty hides it. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetSquadMenu(const FText& MenuText);

	/**
	 * Objective display and, when the mission has ended, the result. Compact by default (the active
	 * objectives); bShowAllObjectives lists every revealed objective with its state (the Tab panel).
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetMissionStatus(const FOutbreakMissionStatus& Status, bool bShowAllObjectives = false);

	/** Stealth awareness meter and alarm line (Phase 13). Empty hides it. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetAwareness(const FText& AwarenessLine);

	/** Squad communication subtitle ("Hawk: Contact!"). Empty hides it. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetSubtitle(const FText& Subtitle);

	/** Hit marker at the crosshair: a hit, or a kill (bigger). */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetHitMarker(bool bVisible, bool bKill);

	/** Objective notifications ("Objective complete: ..."), one per line. Empty hides them. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetNotifications(const FText& Notifications);

	/** Inventory summary, or the full panel when open (Phase 10). */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetInventory(const FText& InventoryText);

	/** Downed / revive status. Empty hides it. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetMedicalStatus(const FText& StatusText);

	/** Matches UOutbreakInteractionComponent::OnFocusChanged so it can be bound directly. */
	UFUNCTION()
	void HandleInteractionFocusChanged(AActor* FocusedActor, const FText& InteractionPrompt);

protected:

	virtual void NativeOnInitialized() override;

	/** Called after the prompt text and visibility change, for Blueprint styling or animation. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnInteractionPromptChanged(const FText& Prompt, bool bPromptVisible);

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Crosshair;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AmmoText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InteractionPromptText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SquadStatusText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SquadMenuText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ObjectiveText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MissionResultText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NotificationText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AwarenessText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HitMarkerText;

	/** Blueprint hook for the hit marker (sound, animation). */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnHitMarker(bool bKill);

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InventoryText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MedicalStatusText;

	/** Shown until real health values are pushed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	FText HealthPlaceholderText;

	/** Shown until real ammunition values are pushed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	FText AmmoPlaceholderText;

private:

	/** Builds crosshair, health, ammo and prompt widgets when the widget has no designer layout. */
	void BuildDefaultLayout();
};
