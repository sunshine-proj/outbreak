#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/OutbreakAIAgent.h"
#include "AI/OutbreakTeammateBehaviorTypes.h"

#include "OutbreakSquadAIController.generated.h"

class UAISenseConfig_Sight;
class UOutbreakHealthComponent;
class UOutbreakSquadComponent;
class UOutbreakSquadMemberComponent;
class UOutbreakTeammateAIConfig;
class UOutbreakWeaponComponent;
class UDamageType;
struct FEnvQueryResult;

/**
 * AI controller for squad teammates (server only).
 *
 * Responsibilities, kept apart (AGENTS.md §6):
 * - Perception: sight via UAIPerceptionComponent; tracks the nearest visible hostile.
 * - Decision: UpdateBehavior gathers context, advances the current order, and picks a behaviour
 *   with the configurable priority rules (OutbreakTeammateBehavior::SelectBehavior).
 * - Positioning: turns the behaviour into a destination (follow slot, hold point, approach point).
 * - Navigation: the Behavior Tree moves to the blackboard MoveLocation; detour crowd avoidance
 *   keeps teammates from bumping into each other.
 * - Combat: UBTService_OutbreakCombat aims, fires and reloads using GetCombatTarget.
 *
 * Decision and combat run from Behavior Tree services at their configured intervals, not every frame.
 */
UCLASS()
class OUTBREAK_API AOutbreakSquadAIController : public AAIController, public IOutbreakAIAgent
{
	GENERATED_BODY()

public:

	AOutbreakSquadAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin IOutbreakAIAgent interface
	/** Re-evaluates threat, order progress and behaviour, and updates the blackboard. */
	virtual void UpdateBehavior() override;

	/** Attack target when attacking, otherwise the current threat. */
	virtual AActor* GetCombatTarget() const override;

	/** Whether Actor is currently seen. */
	virtual bool IsActorVisible(const AActor* Actor) const override;

	/** Maximum firing distance: the role preference if set, otherwise the config. */
	virtual float GetEngagementRange() const override;

	virtual float GetAimTolerance() const override;
	virtual float GetIdleReloadFraction() const override;
	virtual UOutbreakWeaponComponent* GetAgentWeaponComponent() const override { return WeaponComponent; }

	/** The current threat, or its last known location while remembered. */
	virtual bool GetCoverThreatLocation(FVector& OutLocation) const override;

	/** The ordered position for Defend and Cover; otherwise the teammate itself (returns false). */
	virtual bool GetCoverAnchorLocation(FVector& OutLocation) const override;

	/** In squad stealth mode: only when compromised, under fire, or ordered to attack. */
	virtual bool IsFireAllowed() const override;
	//~ End IOutbreakAIAgent interface

	/** Damaged within UnderFireMemory seconds. */
	bool IsUnderFire() const;

	bool HasCover() const { return bHasCover; }

	/** Highest detection of this teammate by any enemy, as of the last update. */
	float GetNoticedLevel() const { return NoticedLevel; }
	FVector GetCoverLocation() const { return CoverLocation; }

	AActor* GetCurrentThreat() const { return CurrentThreat.Get(); }

	EOutbreakTeammateBehavior GetCurrentBehavior() const { return CurrentDecision.Behavior; }
	EOutbreakBehaviorLayer GetCurrentLayer() const { return CurrentDecision.Layer; }

	/** Where the current behaviour wants the teammate to be. */
	FVector GetMoveLocation() const { return MoveLocation; }

	const UOutbreakTeammateAIConfig* GetAIConfig() const;
	UOutbreakSquadMemberComponent* GetSquadMember() const { return SquadMember; }
	UOutbreakWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

protected:

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

private:

	void ApplyPerceptionConfig();
	void UpdateThreat();
	void UpdateOrderProgress();
	FVector ComputeMoveLocation(EOutbreakTeammateBehavior Behavior, bool bBehaviorChanged);
	FVector ComputeFollowLocation() const;
	FVector ComputeDefendLocation() const;
	FVector ComputeRetreatLocation() const;

	/** Living teammates and this teammate's index among them, for slot assignment. */
	void GetSlot(int32& OutSlotIndex, int32& OutSlotCount) const;

	/** Snaps Location to the navmesh. False if a navmesh exists but nothing is near (Location unchanged). */
	bool ProjectToNavigation(FVector& InOutLocation) const;

	double GetTimeSeconds() const;
	bool HasRememberedThreat() const;
	void RequestCoverQueryIfNeeded(bool bNeedCover);
	void HandleCoverQueryResult(TSharedPtr<FEnvQueryResult> Result);

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);
	void WriteBlackboard() const;

	UOutbreakSquadComponent* GetSquad() const;
	APawn* GetPlayerPawn() const;

	UFUNCTION()
	void HandleCommandChanged(UOutbreakSquadMemberComponent* Member);

	UPROPERTY(Transient)
	TObjectPtr<UOutbreakSquadMemberComponent> SquadMember;

	UPROPERTY(Transient)
	TObjectPtr<UOutbreakWeaponComponent> WeaponComponent;

	UPROPERTY(Transient)
	TObjectPtr<UOutbreakHealthComponent> HealthComponent;

	/** Config of the possessed teammate, or the class defaults. Treated as read-only. */
	UPROPERTY(Transient)
	TObjectPtr<UOutbreakTeammateAIConfig> AIConfig;

	TWeakObjectPtr<AActor> CurrentThreat;
	TArray<TWeakObjectPtr<AActor>> VisibleHostiles;

	FOutbreakBehaviorDecision CurrentDecision;
	FVector MoveLocation = FVector::ZeroVector;
	bool bUpdatingBehavior = false;

	// Threat memory and fire.
	FVector LastThreatLocation = FVector::ZeroVector;
	double LastThreatTime = -1.0e9;
	double LastDamageTime = -1.0e9;

	// Cover (EQS results arrive asynchronously).
	FVector CoverLocation = FVector::ZeroVector;
	bool bHasCover = false;
	bool bCoverQueryPending = false;
	double LastCoverQueryTime = -1.0e9;

	FVector RetreatLocation = FVector::ZeroVector;
	bool bHasRetreatLocation = false;

	// Squad stealth.
	void UpdateStealth(bool& bOutShouldHide);
	float NoticedLevel = 0.0f;
	bool bCrouchedForStealth = false;
};
