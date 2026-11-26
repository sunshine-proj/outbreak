#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapons/OutbreakWeaponTypes.h"

#include "OutbreakWeaponComponent.generated.h"

class UOutbreakAmmoTypeDefinition;
class UOutbreakWeaponComponent;
class UOutbreakWeaponDefinition;
struct FOutbreakFireModeDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakWeaponStateChangedSignature, UOutbreakWeaponComponent*, WeaponComponent);

/**
 * Carries weapons and ammunition reserves, and fires, reloads and switches weapons.
 *
 * Server-authoritative (ROADMAP D-006):
 * - Request functions (StartFire, StartReload, EquipWeapon, CycleFireMode, ...) can be called on the
 *   owning client or the server. On a client they forward to Server RPCs.
 * - The server validates every request (fire rate, ammunition, reload state, shot origin), changes
 *   state, applies damage and multicasts cosmetic effects.
 * - Weapon slots, reserves and reload state replicate. OnWeaponStateChanged fires on every machine
 *   when they change.
 * - The owning client applies recoil locally for responsiveness. Ammunition is not predicted:
 *   a remote client sees ammunition change after a round trip.
 *
 * Usable by the player and, from Phase 3, AI teammates (who call the same request functions).
 */
UCLASS(ClassGroup = (Outbreak), meta = (BlueprintSpawnableComponent))
class OUTBREAK_API UOutbreakWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UOutbreakWeaponComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Loadout (authority) ---

	/**
	 * Gives a weapon with a full magazine of its first compatible ammunition type. Equips it if no
	 * weapon is equipped. Returns its slot index, or INDEX_NONE if the definition is invalid.
	 * Giving a weapon already carried returns its existing slot.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Weapons")
	int32 AddWeapon(UOutbreakWeaponDefinition* Definition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Weapons")
	void AddAmmo(UOutbreakAmmoTypeDefinition* AmmoType, int32 Count);

	/** Disabled weapons refuse every request (for example when the owner dies). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Weapons")
	void SetWeaponsEnabled(bool bEnabled);

	// --- Requests (owning client or authority) ---

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void StartReload();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void EquipWeapon(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void EquipNextWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void EquipPreviousWeapon();

	/** Switches the active weapon to its next fire mode, wrapping to the primary mode. */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void CycleFireMode();

	/** Selects the active weapon's next compatible ammunition type and reloads with it. */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void CycleAmmoType();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void SetAiming(bool bNewAiming);

	// --- Queries ---

	UFUNCTION(BlueprintPure, Category = "Weapons")
	int32 GetWeaponCount() const { return Weapons.Num(); }

	UFUNCTION(BlueprintPure, Category = "Weapons")
	int32 GetActiveWeaponIndex() const { return ActiveWeaponIndex; }

	/** Returns false when no weapon is equipped. */
	UFUNCTION(BlueprintPure, Category = "Weapons")
	bool GetActiveWeaponSlot(FOutbreakWeaponSlot& OutSlot) const;

	UFUNCTION(BlueprintPure, Category = "Weapons")
	UOutbreakWeaponDefinition* GetActiveWeaponDefinition() const;

	/** Null when no weapon is equipped. */
	const FOutbreakFireModeDefinition* GetActiveFireMode() const;

	UFUNCTION(BlueprintPure, Category = "Weapons")
	int32 GetAmmoInMagazine() const;

	UFUNCTION(BlueprintPure, Category = "Weapons")
	int32 GetReserveAmmo(const UOutbreakAmmoTypeDefinition* AmmoType) const;

	UFUNCTION(BlueprintPure, Category = "Weapons")
	bool IsReloading() const { return bIsReloading; }

	UFUNCTION(BlueprintPure, Category = "Weapons")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category = "Weapons")
	bool AreWeaponsEnabled() const { return bWeaponsEnabled; }

	/** True when the active weapon can fire a shot now (ignoring fire rate). */
	UFUNCTION(BlueprintPure, Category = "Weapons")
	bool CanFire() const;

	UFUNCTION(BlueprintPure, Category = "Weapons")
	bool CanReload() const;

	/** Rounds moved from reserve to magazine by a reload. */
	static int32 CalculateReloadAmount(int32 MagazineCapacity, int32 AmmoInMagazine, int32 ReserveAmmo);

	/** Fires on every machine when weapons, ammunition, fire mode, reload or aim state change. */
	UPROPERTY(BlueprintAssignable, Category = "Weapons")
	FOutbreakWeaponStateChangedSignature OnWeaponStateChanged;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Weapons given on BeginPlay (authority), in slot order. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TArray<TObjectPtr<UOutbreakWeaponDefinition>> DefaultWeapons;

	/** Reserve ammunition given on BeginPlay (authority). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TArray<FOutbreakAmmoAmount> DefaultAmmoReserves;

	/** Start a reload when trying to fire with too little ammunition in the magazine. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	bool bAutoReloadWhenEmpty = true;

	/** Fraction of the fire interval the server tolerates as network jitter. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Validation", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float FireRateTolerance = 0.05f;

	/** Maximum distance between a client-reported shot origin and the owner's eyes on the server. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Validation", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxShotOriginError = 150.0f;

private:

	// Server RPCs. Each forwards to the matching authority function below.
	UFUNCTION(Server, Reliable)
	void ServerFireShot(FVector_NetQuantize Origin, FVector_NetQuantizeNormal Direction);

	UFUNCTION(Server, Reliable)
	void ServerStartReload();

	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerCycleFireMode();

	UFUNCTION(Server, Reliable)
	void ServerCycleAmmoType();

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bNewAiming);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayShotEffects(UOutbreakWeaponDefinition* Weapon, UOutbreakAmmoTypeDefinition* AmmoType,
		FVector_NetQuantize Origin, FVector_NetQuantizeNormal Direction, const TArray<FOutbreakShotImpact>& Impacts);

	// Authority implementations.
	void AuthFireShot(const FVector& Origin, const FVector& Direction);
	void AuthStartReload();
	void AuthFinishReload();
	void AuthCancelReload();
	void AuthEquipWeapon(int32 SlotIndex);
	void AuthCycleFireMode();
	void AuthCycleAmmoType();

	/** Fires one shot if the local fire rate allows. Called by StartFire and the automatic-fire timer. */
	void FireLocalShot();
	void ApplyLocalRecoil(const FOutbreakFireModeDefinition& FireMode) const;

	bool HasOwnerAuthority() const;
	FOutbreakWeaponSlot* GetActiveSlotMutable();
	const FOutbreakWeaponSlot* GetActiveSlot() const;
	FOutbreakAmmoAmount* FindReserve(const UOutbreakAmmoTypeDefinition* AmmoType);
	void AddReserve(UOutbreakAmmoTypeDefinition* AmmoType, int32 Count);
	void BroadcastStateChanged();

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState)
	TArray<FOutbreakWeaponSlot> Weapons;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState)
	int32 ActiveWeaponIndex = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState)
	TArray<FOutbreakAmmoAmount> AmmoReserves;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState)
	bool bIsReloading = false;

	/** Set locally by the owner; replicated to everyone else. */
	UPROPERTY(ReplicatedUsing = OnRep_WeaponState)
	bool bIsAiming = false;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState)
	bool bWeaponsEnabled = true;

	// Authority-only shot state.
	double LastAuthShotTime = -1.0e9;
	float CurrentBloom = 0.0f;
	FTimerHandle ReloadTimer;

	// Local (requesting machine) fire state.
	double LastLocalShotTime = -1.0e9;
	bool bWantsToFire = false;
	FTimerHandle AutoFireTimer;
};
