#pragma once

#include "CoreMinimal.h"
#include "Environment/OutbreakSwitchable.h"
#include "GameFramework/Actor.h"
#include "Interaction/OutbreakInteractable.h"

#include "OutbreakDoor.generated.h"

class APawn;
class UBoxComponent;
class UNavModifierComponent;
class UOutbreakItemDefinition;
class UPrimitiveComponent;
class USoundBase;
class UStaticMeshComponent;

/**
 * An interactive door (Phase 12): swings or slides open with Interact. Can be locked, opened with a
 * key item, or controlled by equipment (IOutbreakSwitchable: on = unlock and open, off = close
 * and lock).
 *
 * Tactical effects come from existing systems: a closed door blocks sight and muffles noise (its
 * mesh blocks the Visibility channel), and opening or closing it makes a noise enemies can hear.
 *
 * AI: teammates and enemies open unlocked doors by walking into them (bAIOpens). The door's
 * navigation modifier makes the doorway impassable to AI while the door is locked, or closed and
 * not AI-openable, so paths go around. Needs runtime navigation "Dynamic Modifiers Only" or "Dynamic".
 *
 * Server-authoritative; open and locked state replicate. Ticks only while moving.
 */
UCLASS(Blueprintable)
class OUTBREAK_API AOutbreakDoor : public AActor, public IOutbreakInteractable, public IOutbreakSwitchable
{
	GENERATED_BODY()

public:

	AOutbreakDoor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpen() const { return bOpen; }

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsLocked() const { return bLocked; }

	/** Authority. Returns false when locked or already open. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Door")
	bool Open(AActor* OpenedBy);

	/** Authority. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Door")
	void Close(AActor* ClosedBy);

	/** Authority. Locking does not close an open door. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Door")
	void SetLocked(bool bNewLocked);

	/** Authority: the key that unlocks it (null = only equipment or scripts unlock it), and whether unlocking uses the key up. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Door")
	void SetKey(UOutbreakItemDefinition* NewKey, bool bConsume);

	/** Authority: a pawn walked into the door. Opens it for AI-controlled pawns when unlocked and AI may open it. */
	bool TryAutoOpen(APawn* Pawn);

	/** True when AI cannot path through the doorway right now. */
	UFUNCTION(BlueprintPure, Category = "Door")
	bool BlocksNavigation() const;

	/** 0 closed .. 1 open, as currently shown. */
	float GetOpenAlpha() const { return OpenAlpha; }

protected:

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	//~ Begin IOutbreakInteractable interface
	virtual bool CanInteract_Implementation(AActor* InteractionInstigator) const override;
	virtual FText GetInteractionPrompt_Implementation(AActor* InteractionInstigator) const override;
	virtual void Interact_Implementation(AActor* InteractionInstigator) override;
	//~ End IOutbreakInteractable interface

	//~ Begin IOutbreakSwitchable interface
	virtual void SetSwitchedOn_Implementation(bool bOn, AActor* SwitchedBy) override;
	//~ End IOutbreakSwitchable interface

	/** For Blueprint feedback on every machine. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Door")
	void OnDoorStateChanged(bool bNowOpen, bool bNowLocked);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Hinge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/** AI walking into this opens the door. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> AutoOpenTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNavModifierComponent> NavModifier;

	/** Swing (rotate around the hinge) or slide (move sideways). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	bool bSliding = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (EditCondition = "!bSliding", Units = "deg"))
	float OpenAngle = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (EditCondition = "bSliding"))
	FVector SlideOffset = FVector(0.0f, 110.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (ClampMin = "0.05", Units = "s"))
	float OpenTime = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	bool bStartsOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	bool bStartsLocked = false;

	/** Carrying this unlocks the door with Interact. Empty = only equipment or scripts unlock it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UOutbreakItemDefinition> Key;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (EditCondition = "Key != nullptr"))
	bool bConsumeKey = false;

	/** Teammates and enemies open the unlocked door by walking into it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	bool bAIOpens = true;

	/** Seconds after an AI opened it before it closes again once nobody is in the doorway. 0 = stays open. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (ClampMin = "0.0", Units = "s"))
	float AutoCloseDelay = 4.0f;

	/** Gameplay noise of opening or closing (0 = silent). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (ClampMin = "0.0"))
	float NoiseLoudness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Audio")
	TObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Audio")
	TObjectPtr<USoundBase> CloseSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Audio")
	TObjectPtr<USoundBase> LockedSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	FText OpenPrompt;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	FText ClosePrompt;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	FText LockedPrompt;

private:

	bool HasKey(const AActor* InteractionInstigator) const;
	void SetOpen(bool bNewOpen, AActor* Instigator);
	void UpdateNavigation();
	void UpdateVisuals();
	void StartMoving();
	void TryAutoClose();

	UFUNCTION()
	void HandleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_DoorState();

	UPROPERTY(ReplicatedUsing = OnRep_DoorState)
	bool bOpen = false;

	UPROPERTY(ReplicatedUsing = OnRep_DoorState)
	bool bLocked = false;

	float OpenAlpha = 0.0f;
	FTransform ClosedHingeTransform;
	FTimerHandle AutoCloseTimer;
};
