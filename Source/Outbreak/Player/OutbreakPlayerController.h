#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "OutbreakPlayerController.generated.h"

class UOutbreakHealthComponent;
class UOutbreakSquadCommandComponent;
class UOutbreakHUDWidget;
class UOutbreakWeaponComponent;

/**
 * Player controller. Owns the local HUD and connects it to the possessed pawn's components
 * (interaction, health, weapons).
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

private:

	void CreateHUDWidget();
	void BindHUDToPawn(APawn* InPawn);
	void UnbindHUDFromPawn(APawn* InPawn);

	UFUNCTION()
	void HandlePawnHealthChanged(UOutbreakHealthComponent* HealthComponent, float NewHealth, float Delta);

	UFUNCTION()
	void HandlePawnWeaponStateChanged(UOutbreakWeaponComponent* WeaponComponent);

	void RefreshSquadPanel();

	FTimerHandle SquadPanelTimer;

	UPROPERTY(Transient)
	TObjectPtr<UOutbreakHUDWidget> HUDWidget;
};
