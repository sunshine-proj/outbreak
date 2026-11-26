#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "OutbreakMedicalComponent.generated.h"

class UOutbreakMedicalComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakReviveStateChangedSignature, UOutbreakMedicalComponent*, MedicalComponent);

/**
 * Lets a soldier revive incapacitated squadmates (Phase 10). Used by the player (interaction with a
 * downed soldier) and by teammate AI.
 *
 * A revive takes UOutbreakMedicalSettings::ReviveDuration (shortened by the role's revive speed)
 * and is cancelled if the reviver goes down, moves out of ReviveRange, or the patient dies. It needs
 * no item; healing a living soldier uses medical items through the inventory.
 */
UCLASS(ClassGroup = (Outbreak), meta = (BlueprintSpawnableComponent))
class OUTBREAK_API UOutbreakMedicalComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UOutbreakMedicalComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Whether the owner may start reviving Patient now (alive, ally, downed, in range, not busy). */
	UFUNCTION(BlueprintPure, Category = "Medical")
	bool CanRevive(const AActor* Patient) const;

	/** Starts reviving Patient. Owner or authority; a client asks the server. */
	UFUNCTION(BlueprintCallable, Category = "Medical")
	void StartRevive(AActor* Patient);

	/** Owner or authority. */
	UFUNCTION(BlueprintCallable, Category = "Medical")
	void CancelRevive();

	/** Authority: finishes the revive in progress (the revive timer calls this). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Medical")
	void CompleteRevive();

	UFUNCTION(BlueprintPure, Category = "Medical")
	bool IsReviving() const { return RevivePatient != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Medical")
	AActor* GetRevivePatient() const { return RevivePatient; }

	/** 0..1 while reviving; valid on every machine. */
	UFUNCTION(BlueprintPure, Category = "Medical")
	float GetReviveProgress() const;

	/** Seconds a revive by the owner takes (project setting / role revive speed). */
	UFUNCTION(BlueprintPure, Category = "Medical")
	float GetReviveDuration() const;

	/** Who is reviving Patient in World, if anyone (so two helpers don't converge on one patient). */
	static UOutbreakMedicalComponent* FindReviverOf(const UWorld* World, const AActor* Patient);

	/** Fires on every machine when a revive starts, completes or is cancelled. */
	UPROPERTY(BlueprintAssignable, Category = "Medical")
	FOutbreakReviveStateChangedSignature OnReviveStateChanged;

private:

	bool HasOwnerAuthority() const;
	double GetServerTimeSeconds() const;
	void AuthStartRevive(AActor* Patient);
	void AuthCancelRevive();
	void CheckReviveConditions();

	UFUNCTION(Server, Reliable)
	void ServerStartRevive(AActor* Patient);

	UFUNCTION(Server, Reliable)
	void ServerCancelRevive();

	UFUNCTION()
	void OnRep_Revive();

	UPROPERTY(ReplicatedUsing = OnRep_Revive)
	TObjectPtr<AActor> RevivePatient;

	UPROPERTY(ReplicatedUsing = OnRep_Revive)
	double ReviveStartServerTime = 0.0;

	UPROPERTY(ReplicatedUsing = OnRep_Revive)
	float ReviveDurationSeconds = 0.0f;

	FTimerHandle ReviveTimer;
	FTimerHandle ConditionTimer;
};
