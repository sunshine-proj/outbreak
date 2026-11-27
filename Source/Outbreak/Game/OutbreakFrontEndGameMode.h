#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "OutbreakFrontEndGameMode.generated.h"

/**
 * Game mode for the campaign screen level (Phase 14): no pawn, an AOutbreakFrontEndPlayerController
 * showing UOutbreakCampaignMenuWidget. Set it as the front-end map's GameMode Override.
 */
UCLASS()
class OUTBREAK_API AOutbreakFrontEndGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AOutbreakFrontEndGameMode();

protected:

	/** Players stay pawnless on the campaign screen. */
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override {}
};
