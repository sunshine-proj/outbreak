#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "OutbreakPlayerState.generated.h"

class UOutbreakSquadCommsComponent;
class UOutbreakSquadComponent;

/**
 * Per-player state. Holds the player's squad so it survives pawn death and respawn, and replicates
 * to every client (D-009).
 */
UCLASS()
class OUTBREAK_API AOutbreakPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	AOutbreakPlayerState();

	UFUNCTION(BlueprintPure, Category = "Squad")
	UOutbreakSquadComponent* GetSquadComponent() const { return SquadComponent; }

	UFUNCTION(BlueprintPure, Category = "Squad")
	UOutbreakSquadCommsComponent* GetSquadCommsComponent() const { return SquadCommsComponent; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Squad")
	TObjectPtr<UOutbreakSquadComponent> SquadComponent;

	/** Squad communication: callouts and subtitles (Phase 13). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Squad")
	TObjectPtr<UOutbreakSquadCommsComponent> SquadCommsComponent;
};
