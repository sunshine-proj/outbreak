#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"

#include "OutbreakInteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOutbreakInteractionFocusChangedSignature,
	AActor*, FocusedActor, const FText&, InteractionPrompt);

/**
 * Finds the interactable the owner is looking at and performs interactions with it.
 *
 * Detection is a line trace from the owner's eyes view point (the camera for a player-controlled
 * pawn). It runs on a timer at FocusUpdateInterval, not every frame. The focus is only tracked
 * while the owning pawn is locally controlled.
 */
UCLASS(ClassGroup = (Outbreak), meta = (BlueprintSpawnableComponent))
class OUTBREAK_API UOutbreakInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UOutbreakInteractionComponent();

	/** Refreshes the focus, then interacts with the focused actor. Returns true if an interaction happened. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool TryInteract();

	/** Traces for an interactable and updates the focus. Runs automatically on a timer. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void UpdateFocus();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetFocusedPrompt() const { return FocusedPrompt; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	float GetInteractionRange() const { return InteractionRange; }

	/**
	 * Whether Candidate can be focused from DistanceToCandidate: it implements
	 * IOutbreakInteractable, is within InteractionRange and accepts the owner as instigator.
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsValidInteractionTarget(const AActor* Candidate, float DistanceToCandidate) const;

	/** Broadcast when the focused actor or its prompt changes. A null actor means nothing is focused. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOutbreakInteractionFocusChangedSignature OnFocusChanged;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Maximum distance from the eyes to an interactable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0", Units = "cm"))
	float InteractionRange = 200.0f;

	/** Seconds between focus traces. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.01", Units = "s"))
	float FocusUpdateInterval = 0.1f;

	/** Channel used by the focus trace. Interactables must block it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

private:

	AActor* FindFocusCandidate() const;
	void SetFocus(AActor* NewFocus);

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> FocusedActor;

	UPROPERTY(Transient)
	FText FocusedPrompt;

	FTimerHandle FocusUpdateTimer;
};
