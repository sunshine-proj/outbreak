#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "OutbreakPlayerController.generated.h"

class UOutbreakHUDWidget;

/**
 * Player controller. Owns the local HUD and connects it to the possessed pawn's components.
 *
 * Single-player assumption: HUD binding happens in OnPossess, which runs on the authority.
 * Revisit when the replication stance (ROADMAP O-006) is decided.
 */
UCLASS()
class OUTBREAK_API AOutbreakPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AOutbreakPlayerController();

	UFUNCTION(BlueprintPure, Category = "HUD")
	UOutbreakHUDWidget* GetHUDWidget() const { return HUDWidget; }

protected:

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** HUD class to create. Defaults to the C++ HUD with its built-in layout. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UOutbreakHUDWidget> HUDWidgetClass;

private:

	void CreateHUDWidget();
	void BindHUDToPawn(APawn* InPawn);
	void UnbindHUDFromPawn(APawn* InPawn);

	UPROPERTY(Transient)
	TObjectPtr<UOutbreakHUDWidget> HUDWidget;
};
