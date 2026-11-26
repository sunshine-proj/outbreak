#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "OutbreakHealthComponent.generated.h"

class AController;
class UDamageType;
class UOutbreakHealthComponent;

UENUM(BlueprintType)
enum class EOutbreakHealthState : uint8
{
	Alive,
	/** Down but recoverable: bleeds out unless revived (Phase 10). Entered only when bCanBeIncapacitated. */
	Incapacitated,
	Dead
};

/** Multiplies damage of DamageType (and its subclasses). 0 = immune, 1 = normal, 2 = double. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakDamageMultiplier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0.0"))
	float Multiplier = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOutbreakHealthChangedSignature,
	UOutbreakHealthComponent*, HealthComponent, float, NewHealth, float, Delta);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOutbreakHealthStateChangedSignature,
	UOutbreakHealthComponent*, HealthComponent, EOutbreakHealthState, NewState, EOutbreakHealthState, OldState);

/**
 * Reusable health for the player, teammates, enemies and damageable objects.
 *
 * Server-authoritative: damage is accepted only on the authority. Health and state replicate, and
 * the change events fire on every machine (directly on the server, from OnRep on clients), so
 * cosmetic reactions can bind to them anywhere.
 *
 * Damage arrives through the engine damage pipeline (UGameplayStatics::ApplyPointDamage /
 * ApplyRadialDamage / ApplyDamage), which this component receives from its owner's OnTakeAnyDamage.
 */
UCLASS(ClassGroup = (Outbreak), meta = (BlueprintSpawnableComponent))
class OUTBREAK_API UOutbreakHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UOutbreakHealthComponent();

	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	EOutbreakHealthState GetHealthState() const { return HealthState; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsAlive() const { return HealthState == EOutbreakHealthState::Alive; }

	/** Damage after this component's multipliers, before it is subtracted from health. */
	UFUNCTION(BlueprintPure, Category = "Health")
	float CalculateDamage(float RawDamage, const UDamageType* DamageType) const;

	/**
	 * Applies damage on the authority and returns the health actually removed.
	 * Normally called through the engine damage pipeline rather than directly.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	float ApplyDamage(float RawDamage, const UDamageType* DamageType);

	/** Restores full health and the Alive state (respawn, test targets). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void ResetHealth();

	/** Sets the multiplier for DamageType, replacing an existing entry for exactly that class. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void SetDamageMultiplier(TSubclassOf<UDamageType> DamageType, float Multiplier);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void SetCanBeIncapacitated(bool bInCanBeIncapacitated) { bCanBeIncapacitated = bInCanBeIncapacitated; }

	/**
	 * Incapacitation rules (Phase 10, normally from UOutbreakMedicalSettings): bleed-out time
	 * (0 = never) and how many revives this owner can receive (0 = unlimited).
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void ConfigureIncapacitation(bool bInCanBeIncapacitated, float InBleedOutTime, int32 InMaxRevives);

	/** Restores up to Amount health while alive. Returns the health actually restored. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	float Heal(float Amount);

	/** Incapacitated -> Alive with HealthFraction of maximum health. Returns false if not incapacitated. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	bool Revive(float HealthFraction);

	/** Ends an incapacitation in death (the bleed-out timer calls this). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void FinishBleedOut();

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsIncapacitated() const { return HealthState == EOutbreakHealthState::Incapacitated; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthFraction() const { return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Health")
	int32 GetReviveCount() const { return ReviveCount; }

	/** Revives left before zero health kills. -1 = unlimited. */
	UFUNCTION(BlueprintPure, Category = "Health")
	int32 GetRevivesRemaining() const { return MaxRevives > 0 ? FMath::Max(0, MaxRevives - ReviveCount) : -1; }

	/** Seconds until an incapacitated owner bleeds out. -1 when not bleeding out. Valid on every machine. */
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetBleedOutTimeRemaining() const;

	/** Changes MaxHealth (for example from an enemy definition). Optionally refills health. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void SetMaxHealth(float NewMaxHealth, bool bRefillHealth);

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOutbreakHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOutbreakHealthStateChangedSignature OnHealthStateChanged;

protected:

	UPROPERTY(EditDefaultsOnly, Replicated, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	/** If true, reaching zero health incapacitates instead of killing; further damage kills. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	bool bCanBeIncapacitated = false;

	/** Seconds an incapacitated owner lasts. 0 = until revived or finished off. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0.0", Units = "s"))
	float BleedOutTime = 0.0f;

	/** Revives this owner can receive; once used up, zero health kills. 0 = unlimited. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0"))
	int32 MaxRevives = 0;

	/**
	 * Damage multipliers by damage type. Every entry whose DamageType matches the incoming damage
	 * (the same class or a parent class) applies, and their multipliers stack.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	TArray<FOutbreakDamageMultiplier> DamageMultipliers;

private:

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	void SetHealth(float NewHealth);
	void SetHealthState(EOutbreakHealthState NewState);

	UFUNCTION()
	void OnRep_Health(float OldHealth);

	UFUNCTION()
	void OnRep_HealthState(EOutbreakHealthState OldState);

	UPROPERTY(ReplicatedUsing = OnRep_Health)
	float Health = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_HealthState)
	EOutbreakHealthState HealthState = EOutbreakHealthState::Alive;

	UPROPERTY(Replicated)
	int32 ReviveCount = 0;

	/** Server world time of the bleed-out; < 0 when not bleeding out. */
	UPROPERTY(Replicated)
	double BleedOutServerTime = -1.0;

	FTimerHandle BleedOutTimer;

	double GetServerTimeSeconds() const;
	void StartBleedOut();
	void StopBleedOut();
};
