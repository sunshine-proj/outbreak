#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "OutbreakMissionDefinition.generated.h"

class UOutbreakObjective;
class UOutbreakSquadConfiguration;

/**
 * A mission: its squad, its objectives in order, and its failure rules. Set on the game mode
 * (AOutbreakGameMode::MissionDefinition). Phase 11 extends this with briefing, objective
 * dependencies and extraction.
 */
UCLASS(BlueprintType, Const)
class OUTBREAK_API UOutbreakMissionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	bool Validate(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission", meta = (MultiLine = "true"))
	FText Briefing;

	/** The mission's squad. Empty = the game mode's or project default configuration. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Squad")
	TObjectPtr<UOutbreakSquadConfiguration> SquadConfiguration;

	/** Completed one after another; the mission is complete when the last one is. */
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Objectives")
	TArray<TObjectPtr<UOutbreakObjective>> Objectives;

	/** Fail when every player is dead (in single-player: when the player dies). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Failure")
	bool bFailOnPlayerDeath = true;

	/** Seconds the outcome is shown before the level restarts. 0 = no restart. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flow", meta = (ClampMin = "0.0", Units = "s"))
	float RestartDelay = 8.0f;

	/** Seconds between objective evaluations. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flow", meta = (ClampMin = "0.05", Units = "s"))
	float UpdateInterval = 0.25f;
};
