#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Missions/OutbreakMissionTypes.h"
#include "Squad/OutbreakSquadComms.h"
#include "Squad/OutbreakSquadSelection.h"

#include "OutbreakPlayerController.generated.h"

class AOutbreakGameState;
class UOutbreakBriefingWidget;
class UOutbreakHealthComponent;
class UOutbreakMissionSummaryWidget;
class UOutbreakSquadCommsComponent;
class UOutbreakSquadMemberComponent;
class UOutbreakSquadCommandComponent;
class UOutbreakHUDWidget;
class UOutbreakWeaponComponent;

/**
 * Player controller. Owns the local HUD and connects it to the possessed pawn's components
 * (interaction, health, weapons), and runs the local side of the mission flow (Phase 11): the
 * briefing screen until this player deploys, objective notifications, and the summary screen when
 * the mission ends. Both screens follow the replicated mission status in AOutbreakGameState.
 *
 * Binding happens in SetPawn, which runs on the server when possessing and on the owning client
 * when the pawn replicates, so the HUD works for listen-server and remote players (D-006).
 */
UCLASS()
class OUTBREAK_API AOutbreakPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AOutbreakPlayerController();

	UFUNCTION(BlueprintPure, Category = "HUD")
	UOutbreakHUDWidget* GetHUDWidget() const { return HUDWidget; }

	UFUNCTION(BlueprintPure, Category = "Squad")
	UOutbreakSquadCommandComponent* GetSquadCommandComponent() const { return SquadCommandComponent; }

	/** Opens or closes the full inventory panel (the compact summary is always shown). */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleInventoryPanel();

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsInventoryPanelOpen() const { return bInventoryPanelOpen; }

	// --- Mission flow (Phase 11) ---

	/** Leaves the briefing with Selection; the server builds the squad and deploys this player. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void RequestDeploy(const FOutbreakDeploymentSelection& Selection);

	/** Replays the mission after it has ended (the summary's Retry); in a campaign, optionally from the last checkpoint. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void RequestRestartMission(bool bFromCheckpoint = false);

	/** The summary's Continue: the next campaign mission (Phase 14). */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void RequestContinueCampaign();

	/**
	 * The summary's Quit / Main Menu: in a campaign the host returns everyone to the campaign
	 * screen; outside one, or on a co-op client, the game quits.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void QuitMission();

	/** Highest detection of this player's pawn by any enemy, 0..1 (replicated from the server, Phase 13). */
	UFUNCTION(BlueprintPure, Category = "Stealth")
	float GetEnemyAwareness() const { return EnemyAwareness / 100.0f; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Squad")
	TObjectPtr<UOutbreakSquadCommandComponent> SquadCommandComponent;

	/** Seconds between squad panel and objective refreshes (distances change continuously; no per-frame update). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (ClampMin = "0.05", Units = "s"))
	float SquadPanelRefreshInterval = 0.25f;

public:

	virtual void SetPawn(APawn* InPawn) override;

protected:

	/** HUD class to create. Defaults to the C++ HUD with its built-in layout. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UOutbreakHUDWidget> HUDWidgetClass;

	/** Briefing screen class. Defaults to the C++ screen with its built-in layout. */
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TSubclassOf<UOutbreakBriefingWidget> BriefingWidgetClass;

	/** Mission summary screen class. Defaults to the C++ screen with its built-in layout. */
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TSubclassOf<UOutbreakMissionSummaryWidget> MissionSummaryWidgetClass;

	/** Seconds each objective notification stays on screen. */
	UPROPERTY(EditDefaultsOnly, Category = "Mission", meta = (ClampMin = "0.5", Units = "s"))
	float NotificationDuration = 4.0f;

	/** Most notifications shown at once; older ones are dropped. */
	UPROPERTY(EditDefaultsOnly, Category = "Mission", meta = (ClampMin = "1"))
	int32 MaxNotifications = 3;

	/** Seconds the hit marker shows after a hit. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (ClampMin = "0.05", Units = "s"))
	float HitMarkerDuration = 0.25f;

	/** Seconds between server updates of the stealth awareness meter. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (ClampMin = "0.05", Units = "s"))
	float AwarenessUpdateInterval = 0.25f;

private:

	void CreateHUDWidget();
	void BindHUDToPawn(APawn* InPawn);
	void UnbindHUDFromPawn(APawn* InPawn);

	UFUNCTION()
	void HandlePawnHealthChanged(UOutbreakHealthComponent* HealthComponent, float NewHealth, float Delta);

	UFUNCTION()
	void HandlePawnWeaponStateChanged(UOutbreakWeaponComponent* WeaponComponent);

	void RefreshSquadPanel();

	UFUNCTION(Server, Reliable)
	void ServerRequestDeploy(const FOutbreakDeploymentSelection& Selection);

	UFUNCTION(Server, Reliable)
	void ServerRequestRestartMission(bool bFromCheckpoint);

	UFUNCTION(Server, Reliable)
	void ServerRequestContinueCampaign();

	void BindToGameState();

	UFUNCTION()
	void HandleMissionStatusChanged(const FOutbreakMissionStatus& Status);

	/** Shows or hides the briefing and summary screens for Status. */
	void UpdateMissionScreens(const FOutbreakMissionStatus& Status);
	void ShowBriefing();
	void HideBriefing();
	void ShowMissionSummary();
	void RefreshNotifications();
	void ApplyInputMode();

	UFUNCTION()
	void HandleBriefingDeployRequested(const FOutbreakDeploymentSelection& Selection);

	UFUNCTION()
	void HandleSummaryRetry();

	UFUNCTION()
	void HandleSummaryQuit();

	UFUNCTION()
	void HandleSummaryContinue();

	UFUNCTION()
	void HandleSummaryCheckpoint();

	UPROPERTY(Transient)
	TObjectPtr<UOutbreakBriefingWidget> BriefingWidget;

	UPROPERTY(Transient)
	TObjectPtr<UOutbreakMissionSummaryWidget> MissionSummaryWidget;

	TWeakObjectPtr<AOutbreakGameState> BoundGameState;
	FOutbreakMissionStatus LastMissionStatus;
	TArray<TPair<FText, double>> Notifications;
	bool bDeployRequested = false;

	// --- HUD polish (Phase 13) ---

	UFUNCTION(Client, Unreliable)
	void ClientShowHitMarker(bool bKill);

	void HandleDamageDealt(AActor* Victim, AController* DamageInstigator, float Damage, bool bTakenDown);
	void HideHitMarker();
	void UpdateEnemyAwareness();
	void BindToSquadComms();

	UFUNCTION()
	void HandleCallout(UOutbreakSquadMemberComponent* Speaker, EOutbreakCallout Callout, const FText& Subtitle);

	/** Percent, so it replicates as one byte. */
	UPROPERTY(Replicated)
	uint8 EnemyAwareness = 0;

	FDelegateHandle DamageDealtHandle;
	FTimerHandle AwarenessTimer;
	FTimerHandle HitMarkerTimer;
	TWeakObjectPtr<UOutbreakSquadCommsComponent> BoundComms;
	double SubtitleExpiry = 0.0;

	FTimerHandle SquadPanelTimer;
	bool bInventoryPanelOpen = false;

	UPROPERTY(Transient)
	TObjectPtr<UOutbreakHUDWidget> HUDWidget;
};
