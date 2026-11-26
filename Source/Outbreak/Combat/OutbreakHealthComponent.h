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
	/** Down but recoverable. Entered only when bCanBeIncapacitated; recovery gameplay is Phase 10. */
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

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOutbreakHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOutbreakHealthStateChangedSignature OnHealthStateChanged;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	/** If true, reaching zero health incapacitates instead of killing; further damage kills. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	bool bCanBeIncapacitated = false;

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
};
