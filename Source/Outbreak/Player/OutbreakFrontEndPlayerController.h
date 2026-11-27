#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "OutbreakFrontEndPlayerController.generated.h"

class UOutbreakCampaignMenuWidget;

/** Shows the campaign screen with a mouse cursor and UI-only input (Phase 14). */
UCLASS()
class OUTBREAK_API AOutbreakFrontEndPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AOutbreakFrontEndPlayerController();

protected:

	virtual void BeginPlay() override;

	/** The campaign screen. A Widget Blueprint subclass replaces the default layout. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UOutbreakCampaignMenuWidget> CampaignMenuWidgetClass;

private:

	UPROPERTY(Transient)
	TObjectPtr<UOutbreakCampaignMenuWidget> CampaignMenuWidget;
};
