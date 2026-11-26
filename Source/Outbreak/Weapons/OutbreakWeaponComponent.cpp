#include "Weapons/OutbreakWeaponComponent.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Outbreak.h"
#include "Stealth/OutbreakNoiseTypes.h"
#include "TimerManager.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakFireBehavior.h"
#include "Weapons/OutbreakWeaponDefinition.h"
#include "Weapons/OutbreakWeaponEffects.h"

UOutbreakWeaponComponent::UOutbreakWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UOutbreakWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UOutbreakWeaponComponent, Weapons);
	DOREPLIFETIME(UOutbreakWeaponComponent, ActiveWeaponIndex);
	DOREPLIFETIME(UOutbreakWeaponComponent, AmmoReserves);
	DOREPLIFETIME(UOutbreakWeaponComponent, bIsReloading);
	DOREPLIFETIME_CONDITION(UOutbreakWeaponComponent, bIsAiming, COND_SkipOwner);
	DOREPLIFETIME(UOutbreakWeaponComponent, bWeaponsEnabled);
}

void UOutbreakWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (HasOwnerAuthority())
	{
		for (const FOutbreakAmmoAmount& Reserve : DefaultAmmoReserves)
		{
			AddAmmo(Reserve.AmmoType, Reserve.Count);
		}
		for (UOutbreakWeaponDefinition* Definition : DefaultWeapons)
		{
			AddWeapon(Definition);
		}
	}
}

void UOutbreakWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoFireTimer);
		World->GetTimerManager().ClearTimer(ReloadTimer);
	}

	Super::EndPlay(EndPlayReason);
}

// --- Loadout -------------------------------------------------------------------------------------

int32 UOutbreakWeaponComponent::AddWeapon(UOutbreakWeaponDefinition* Definition)
{
	if (!HasOwnerAuthority() || !Definition)
	{
		return INDEX_NONE;
	}

	TArray<FText> Errors;
	if (!Definition->Validate(Errors))
	{
		for (const FText& Error : Errors)
		{
			UE_LOG(LogOutbreak, Warning, TEXT("%s: rejected weapon %s: %s"), *GetNameSafe(GetOwner()),
				*Definition->GetName(), *Error.ToString());
		}
		return INDEX_NONE;
	}

	const int32 ExistingIndex = Weapons.IndexOfByPredicate(
		[Definition](const FOutbreakWeaponSlot& Slot) { return Slot.Definition == Definition; });
	if (ExistingIndex != INDEX_NONE)
	{
		return ExistingIndex;
	}

	FOutbreakWeaponSlot& Slot = Weapons.AddDefaulted_GetRef();
	Slot.Definition = Definition;
	Slot.LoadedAmmoType = Definition->CompatibleAmmoTypes[0];
	Slot.SelectedAmmoType = Slot.LoadedAmmoType;
	Slot.AmmoInMagazine = Definition->MagazineCapacity;
	Slot.FireModeIndex = 0;

	const int32 NewIndex = Weapons.Num() - 1;
	if (ActiveWeaponIndex == INDEX_NONE)
	{
		AuthEquipWeapon(NewIndex);
	}
	else
	{
		BroadcastStateChanged();
	}
	return NewIndex;
}

void UOutbreakWeaponComponent::AddAmmo(UOutbreakAmmoTypeDefinition* AmmoType, int32 Count)
{
	if (!HasOwnerAuthority() || !AmmoType || Count <= 0)
	{
		return;
	}

	AddReserve(AmmoType, Count);
	BroadcastStateChanged();
}

bool UOutbreakWeaponComponent::RemoveWeapon(UOutbreakWeaponDefinition* Definition)
{
	if (!HasOwnerAuthority() || !Definition)
	{
		return false;
	}
	const int32 Index = Weapons.IndexOfByPredicate(
		[Definition](const FOutbreakWeaponSlot& Slot) { return Slot.Definition == Definition; });
	if (Index == INDEX_NONE)
	{
		return false;
	}

	if (Index == ActiveWeaponIndex)
	{
		StopFire();
		AuthCancelReload();
	}

	const FOutbreakWeaponSlot& Slot = Weapons[Index];
	if (Slot.LoadedAmmoType && Slot.AmmoInMagazine > 0 && !Definition->bUnlimitedAmmo)
	{
		AddReserve(Slot.LoadedAmmoType, Slot.AmmoInMagazine);
	}
	Weapons.RemoveAt(Index);

	if (Weapons.IsEmpty())
	{
		ActiveWeaponIndex = INDEX_NONE;
	}
	else if (Index < ActiveWeaponIndex)
	{
		--ActiveWeaponIndex;
	}
	else if (Index == ActiveWeaponIndex)
	{
		ActiveWeaponIndex = FMath::Min(Index, Weapons.Num() - 1);
		CurrentBloom = 0.0f;
	}
	BroadcastStateChanged();
	return true;
}

int32 UOutbreakWeaponComponent::RemoveAmmo(UOutbreakAmmoTypeDefinition* AmmoType, int32 Count)
{
	FOutbreakAmmoAmount* Reserve = HasOwnerAuthority() && Count > 0 ? FindReserve(AmmoType) : nullptr;
	if (!Reserve)
	{
		return 0;
	}
	const int32 Removed = FMath::Min(Count, Reserve->Count);
	Reserve->Count -= Removed;
	if (Removed > 0)
	{
		BroadcastStateChanged();
	}
	return Removed;
}

void UOutbreakWeaponComponent::ClearLoadout()
{
	if (!HasOwnerAuthority())
	{
		return;
	}
	StopFire();
	AuthCancelReload();
	Weapons.Reset();
	AmmoReserves.Reset();
	ActiveWeaponIndex = INDEX_NONE;
	CurrentBloom = 0.0f;
	BroadcastStateChanged();
}

int32 UOutbreakWeaponComponent::GetTotalAmmo(const UOutbreakAmmoTypeDefinition* AmmoType) const
{
	int32 Total = GetReserveAmmo(AmmoType);
	for (const FOutbreakWeaponSlot& Slot : Weapons)
	{
		if (Slot.LoadedAmmoType.Get() == AmmoType && Slot.Definition && !Slot.Definition->bUnlimitedAmmo)
		{
			Total += Slot.AmmoInMagazine;
		}
	}
	return Total;
}

TArray<UOutbreakWeaponDefinition*> UOutbreakWeaponComponent::GetWeaponDefinitions() const
{
	TArray<UOutbreakWeaponDefinition*> Result;
	for (const FOutbreakWeaponSlot& Slot : Weapons)
	{
		if (Slot.Definition)
		{
			Result.Add(Slot.Definition);
		}
	}
	return Result;
}

void UOutbreakWeaponComponent::SetWeaponsEnabled(bool bEnabled)
{
	if (!HasOwnerAuthority() || bWeaponsEnabled == bEnabled)
	{
		return;
	}

	bWeaponsEnabled = bEnabled;
	if (!bWeaponsEnabled)
	{
		StopFire();
		AuthCancelReload();
	}
	BroadcastStateChanged();
}

// --- Firing --------------------------------------------------------------------------------------

void UOutbreakWeaponComponent::StartFire()
{
	bWantsToFire = true;
	FireLocalShot();

	const FOutbreakFireModeDefinition* FireMode = GetActiveFireMode();
	if (bWantsToFire && FireMode && FireMode->bAutomatic && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(AutoFireTimer, this, &ThisClass::FireLocalShot,
			FireMode->GetFireInterval(), /*bLoop=*/ true);
	}
}

void UOutbreakWeaponComponent::StopFire()
{
	bWantsToFire = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoFireTimer);
	}
}

void UOutbreakWeaponComponent::FireLocalShot()
{
	const FOutbreakFireModeDefinition* FireMode = GetActiveFireMode();
	const UWorld* World = GetWorld();
	if (!FireMode || !World)
	{
		StopFire();
		return;
	}

	if (!CanFire())
	{
		const FOutbreakWeaponSlot* Slot = GetActiveSlot();
		const bool bEmpty = Slot && Slot->AmmoInMagazine < FireMode->AmmoPerShot;
		if (bEmpty && (!bAutoReloadWhenEmpty || !CanReload()) && !bIsReloading)
		{
			// Click: empty and nothing to reload with. Local feedback only.
			BroadcastWeaponAction(EOutbreakWeaponAction::DryFired, Slot->Definition);
		}
		if (bAutoReloadWhenEmpty && bEmpty)
		{
			StartReload();
		}
		StopFire();
		return;
	}

	const double Now = World->GetTimeSeconds();
	// Same jitter tolerance as the server, so timer-driven automatic fire never drops a shot.
	if (Now - LastLocalShotTime < FireMode->GetFireInterval() * (1.0f - FireRateTolerance))
	{
		return;
	}
	LastLocalShotTime = Now;

	FVector Origin;
	FRotator ViewRotation;
	GetOwner()->GetActorEyesViewPoint(Origin, ViewRotation);
	const FVector Direction = ViewRotation.Vector();

	if (HasOwnerAuthority())
	{
		AuthFireShot(Origin, Direction);
	}
	else
	{
		ServerFireShot(Origin, Direction);
	}

	ApplyLocalRecoil(*FireMode);
}

void UOutbreakWeaponComponent::ServerFireShot_Implementation(FVector_NetQuantize Origin, FVector_NetQuantizeNormal Direction)
{
	AuthFireShot(Origin, Direction);
}

void UOutbreakWeaponComponent::AuthFireShot(const FVector& Origin, const FVector& Direction)
{
	FOutbreakWeaponSlot* Slot = GetActiveSlotMutable();
	const FOutbreakFireModeDefinition* FireMode = GetActiveFireMode();
	UWorld* World = GetWorld();
	if (!Slot || !FireMode || !World || !CanFire())
	{
		return;
	}

	// Fire rate.
	const double Now = World->GetTimeSeconds();
	const float FireInterval = FireMode->GetFireInterval();
	if (Now - LastAuthShotTime < FireInterval * (1.0f - FireRateTolerance))
	{
		UE_LOG(LogOutbreak, Verbose, TEXT("%s: shot rejected, fire rate exceeded."), *GetNameSafe(GetOwner()));
		return;
	}

	// Client-provided aim: the origin must be close to the owner's eyes, the direction non-zero.
	FVector EyesLocation;
	FRotator EyesRotation;
	GetOwner()->GetActorEyesViewPoint(EyesLocation, EyesRotation);
	const FVector AimDirection = Direction.GetSafeNormal();
	if (AimDirection.IsNearlyZero() || FVector::Dist(Origin, EyesLocation) > MaxShotOriginError)
	{
		UE_LOG(LogOutbreak, Warning, TEXT("%s: shot rejected, invalid origin or direction."), *GetNameSafe(GetOwner()));
		return;
	}

	// Commit the shot.
	if (!Slot->Definition->bUnlimitedAmmo)
	{
		Slot->AmmoInMagazine -= FireMode->AmmoPerShot;
	}

	const float Elapsed = static_cast<float>(Now - LastAuthShotTime);
	LastAuthShotTime = Now;
	CurrentBloom = FireMode->Spread.RecoverBloom(CurrentBloom, Elapsed);
	const float SpreadHalfAngle = FMath::DegreesToRadians(FireMode->Spread.GetSpread(bIsAiming, CurrentBloom));
	CurrentBloom += FireMode->Spread.SpreadPerShot;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const UOutbreakAmmoTypeDefinition* AmmoType = Slot->LoadedAmmoType;

	FOutbreakShotRequest Request;
	Request.World = World;
	Request.InstigatorPawn = OwnerPawn;
	Request.InstigatorController = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	Request.DamageCauser = GetOwner();
	Request.AmmoType = AmmoType;
	Request.Origin = Origin;
	Request.Range = FireMode->Range;
	Request.ImpactDamage.Damage = FireMode->BaseDamage * (AmmoType ? AmmoType->DamageMultiplier : 1.0f);
	Request.ImpactDamage.DamageType = AmmoType ? AmmoType->DamageType : nullptr;
	Request.ImpactDamage.ExplosionRadius = AmmoType ? AmmoType->ExplosionRadius : 0.0f;
	for (int32 Index = 0; Index < FireMode->ProjectilesPerShot; ++Index)
	{
		Request.Directions.Add(SpreadHalfAngle > 0.0f ? FMath::VRandCone(AimDirection, SpreadHalfAngle) : AimDirection);
	}

	TArray<FOutbreakShotImpact> Impacts;
	FireMode->FireBehavior->ExecuteShot(Request, Impacts);

	MulticastPlayShotEffects(Slot->Definition, Slot->LoadedAmmoType, Origin, AimDirection, Impacts);

	// Gameplay noise for AI hearing (Phase 8 noise system).
	FOutbreakNoiseEvent Noise;
	Noise.Type = EOutbreakNoiseType::Weapon;
	Noise.Location = Origin;
	Noise.Loudness = Slot->Definition->FireNoiseLoudness;
	Noise.RangeOverride = Slot->Definition->FireNoiseRange;
	Noise.Instigator = GetOwner();
	OutbreakNoise::ReportNoise(World, Noise);

	OnShotFired.Broadcast(this);
	BroadcastStateChanged();
}

void UOutbreakWeaponComponent::MulticastPlayShotEffects_Implementation(UOutbreakWeaponDefinition* Weapon,
	UOutbreakAmmoTypeDefinition* AmmoType, FVector_NetQuantize Origin, FVector_NetQuantizeNormal Direction,
	const TArray<FOutbreakShotImpact>& Impacts)
{
	OutbreakWeaponEffects::PlayMuzzleEffects(this, Weapon, Origin, Direction);
	OnWeaponAction.Broadcast(this, EOutbreakWeaponAction::Fired, Weapon);
	for (const FOutbreakShotImpact& Impact : Impacts)
	{
		if (Impact.bBlockingHit)
		{
			OutbreakWeaponEffects::PlayImpactEffects(this, AmmoType, Impact.Location, Impact.Normal);
		}
	}
}

void UOutbreakWeaponComponent::ApplyLocalRecoil(const FOutbreakFireModeDefinition& FireMode) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* Controller = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	// View recoil is for local human players only; AI aim is handled by its controller.
	if (!Controller || !Controller->IsLocalPlayerController())
	{
		return;
	}

	const float Scale = bIsAiming ? FireMode.Recoil.AimMultiplier : 1.0f;
	const FRotator Kick(
		FireMode.Recoil.PitchKick * Scale,
		FMath::FRandRange(-FireMode.Recoil.YawKickMax, FireMode.Recoil.YawKickMax) * Scale,
		0.0f);
	Controller->SetControlRotation(Controller->GetControlRotation() + Kick);
}

// --- Reloading -----------------------------------------------------------------------------------

int32 UOutbreakWeaponComponent::CalculateReloadAmount(int32 MagazineCapacity, int32 AmmoInMagazine, int32 ReserveAmmo)
{
	const int32 Missing = FMath::Max(0, MagazineCapacity - AmmoInMagazine);
	return FMath::Clamp(ReserveAmmo, 0, Missing);
}

bool UOutbreakWeaponComponent::CanReload() const
{
	const FOutbreakWeaponSlot* Slot = GetActiveSlot();
	if (!bWeaponsEnabled || bIsReloading || !Slot || !Slot->Definition)
	{
		return false;
	}

	const UOutbreakAmmoTypeDefinition* Selected = Slot->SelectedAmmoType;
	if (!Slot->Definition->IsAmmoTypeCompatible(Selected) || GetReserveAmmo(Selected) <= 0)
	{
		return false;
	}

	const bool bSwitchingAmmoType = Selected != Slot->LoadedAmmoType.Get();
	return bSwitchingAmmoType || Slot->AmmoInMagazine < Slot->Definition->MagazineCapacity;
}

void UOutbreakWeaponComponent::StartReload()
{
	if (HasOwnerAuthority())
	{
		AuthStartReload();
	}
	else if (CanReload())
	{
		ServerStartReload();
	}
}

void UOutbreakWeaponComponent::ServerStartReload_Implementation()
{
	AuthStartReload();
}

void UOutbreakWeaponComponent::AuthStartReload()
{
	const FOutbreakWeaponSlot* Slot = GetActiveSlot();
	if (!CanReload() || !Slot)
	{
		return;
	}

	bIsReloading = true;
	BroadcastStateChanged();

	const float Duration = Slot->Definition->ReloadDuration;
	if (Duration <= 0.0f || !GetWorld())
	{
		AuthFinishReload();
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &ThisClass::AuthFinishReload, Duration, /*bLoop=*/ false);
}

void UOutbreakWeaponComponent::AuthFinishReload()
{
	FOutbreakWeaponSlot* Slot = GetActiveSlotMutable();
	if (!bIsReloading || !Slot || !Slot->Definition)
	{
		bIsReloading = false;
		return;
	}

	UOutbreakAmmoTypeDefinition* Selected = Slot->SelectedAmmoType;

	// Switching ammunition type: return the loaded rounds to their own reserve first.
	if (Selected != Slot->LoadedAmmoType.Get())
	{
		if (Slot->LoadedAmmoType && Slot->AmmoInMagazine > 0)
		{
			AddReserve(Slot->LoadedAmmoType, Slot->AmmoInMagazine);
		}
		Slot->AmmoInMagazine = 0;
		Slot->LoadedAmmoType = Selected;
	}

	if (FOutbreakAmmoAmount* Reserve = FindReserve(Selected))
	{
		const int32 Amount = CalculateReloadAmount(Slot->Definition->MagazineCapacity, Slot->AmmoInMagazine, Reserve->Count);
		Slot->AmmoInMagazine += Amount;
		Reserve->Count -= Amount;
	}

	bIsReloading = false;
	BroadcastStateChanged();
}

void UOutbreakWeaponComponent::AuthCancelReload()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadTimer);
	}
	if (bIsReloading)
	{
		bIsReloading = false;
		BroadcastStateChanged();
	}
}

// --- Switching -----------------------------------------------------------------------------------

void UOutbreakWeaponComponent::EquipWeapon(int32 SlotIndex)
{
	if (!Weapons.IsValidIndex(SlotIndex) || SlotIndex == ActiveWeaponIndex)
	{
		return;
	}

	StopFire();
	if (HasOwnerAuthority())
	{
		AuthEquipWeapon(SlotIndex);
	}
	else
	{
		ServerEquipWeapon(SlotIndex);
	}
}

void UOutbreakWeaponComponent::EquipNextWeapon()
{
	if (Weapons.Num() > 1)
	{
		EquipWeapon((ActiveWeaponIndex + 1) % Weapons.Num());
	}
}

void UOutbreakWeaponComponent::EquipPreviousWeapon()
{
	if (Weapons.Num() > 1)
	{
		EquipWeapon((ActiveWeaponIndex - 1 + Weapons.Num()) % Weapons.Num());
	}
}

void UOutbreakWeaponComponent::ServerEquipWeapon_Implementation(int32 SlotIndex)
{
	AuthEquipWeapon(SlotIndex);
}

void UOutbreakWeaponComponent::AuthEquipWeapon(int32 SlotIndex)
{
	if (!bWeaponsEnabled || !Weapons.IsValidIndex(SlotIndex) || SlotIndex == ActiveWeaponIndex)
	{
		return;
	}

	AuthCancelReload();
	ActiveWeaponIndex = SlotIndex;
	CurrentBloom = 0.0f;
	BroadcastStateChanged();
}

void UOutbreakWeaponComponent::CycleFireMode()
{
	StopFire();
	if (HasOwnerAuthority())
	{
		AuthCycleFireMode();
	}
	else
	{
		ServerCycleFireMode();
	}
}

void UOutbreakWeaponComponent::ServerCycleFireMode_Implementation()
{
	AuthCycleFireMode();
}

void UOutbreakWeaponComponent::AuthCycleFireMode()
{
	FOutbreakWeaponSlot* Slot = GetActiveSlotMutable();
	if (!bWeaponsEnabled || !Slot || !Slot->Definition || Slot->Definition->FireModes.Num() < 2)
	{
		return;
	}

	Slot->FireModeIndex = (Slot->FireModeIndex + 1) % Slot->Definition->FireModes.Num();
	CurrentBloom = 0.0f;
	BroadcastStateChanged();
}

void UOutbreakWeaponComponent::CycleAmmoType()
{
	StopFire();
	if (HasOwnerAuthority())
	{
		AuthCycleAmmoType();
	}
	else
	{
		ServerCycleAmmoType();
	}
}

void UOutbreakWeaponComponent::ServerCycleAmmoType_Implementation()
{
	AuthCycleAmmoType();
}

void UOutbreakWeaponComponent::AuthCycleAmmoType()
{
	FOutbreakWeaponSlot* Slot = GetActiveSlotMutable();
	if (!bWeaponsEnabled || bIsReloading || !Slot || !Slot->Definition)
	{
		return;
	}

	const TArray<TObjectPtr<UOutbreakAmmoTypeDefinition>>& Compatible = Slot->Definition->CompatibleAmmoTypes;
	if (Compatible.Num() < 2)
	{
		return;
	}

	const int32 CurrentIndex = Compatible.IndexOfByKey(Slot->SelectedAmmoType);
	Slot->SelectedAmmoType = Compatible[(CurrentIndex + 1) % Compatible.Num()];
	BroadcastStateChanged();

	AuthStartReload();
}

// --- Aiming --------------------------------------------------------------------------------------

void UOutbreakWeaponComponent::SetAiming(bool bNewAiming)
{
	if (bIsAiming == bNewAiming)
	{
		return;
	}

	// Applied locally at once for a responsive camera; the server copy drives spread.
	bIsAiming = bNewAiming;
	BroadcastStateChanged();

	if (!HasOwnerAuthority())
	{
		ServerSetAiming(bNewAiming);
	}
}

void UOutbreakWeaponComponent::ServerSetAiming_Implementation(bool bNewAiming)
{
	if (bIsAiming != bNewAiming)
	{
		bIsAiming = bNewAiming;
		BroadcastStateChanged();
	}
}

// --- Queries and helpers -------------------------------------------------------------------------

bool UOutbreakWeaponComponent::GetActiveWeaponSlot(FOutbreakWeaponSlot& OutSlot) const
{
	if (const FOutbreakWeaponSlot* Slot = GetActiveSlot())
	{
		OutSlot = *Slot;
		return true;
	}
	return false;
}

UOutbreakWeaponDefinition* UOutbreakWeaponComponent::GetActiveWeaponDefinition() const
{
	const FOutbreakWeaponSlot* Slot = GetActiveSlot();
	return Slot ? Slot->Definition.Get() : nullptr;
}

const FOutbreakFireModeDefinition* UOutbreakWeaponComponent::GetActiveFireMode() const
{
	const FOutbreakWeaponSlot* Slot = GetActiveSlot();
	return Slot && Slot->Definition ? Slot->Definition->GetFireMode(Slot->FireModeIndex) : nullptr;
}

int32 UOutbreakWeaponComponent::GetAmmoInMagazine() const
{
	const FOutbreakWeaponSlot* Slot = GetActiveSlot();
	return Slot ? Slot->AmmoInMagazine : 0;
}

int32 UOutbreakWeaponComponent::GetReserveAmmo(const UOutbreakAmmoTypeDefinition* AmmoType) const
{
	const FOutbreakAmmoAmount* Reserve = AmmoReserves.FindByPredicate(
		[AmmoType](const FOutbreakAmmoAmount& Entry) { return Entry.AmmoType.Get() == AmmoType; });
	return Reserve ? Reserve->Count : 0;
}

bool UOutbreakWeaponComponent::CanFire() const
{
	const FOutbreakWeaponSlot* Slot = GetActiveSlot();
	const FOutbreakFireModeDefinition* FireMode = GetActiveFireMode();
	return bWeaponsEnabled
		&& !bIsReloading
		&& Slot
		&& FireMode
		&& FireMode->FireBehavior
		&& Slot->AmmoInMagazine >= FireMode->AmmoPerShot;
}

bool UOutbreakWeaponComponent::HasOwnerAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

FOutbreakWeaponSlot* UOutbreakWeaponComponent::GetActiveSlotMutable()
{
	return Weapons.IsValidIndex(ActiveWeaponIndex) ? &Weapons[ActiveWeaponIndex] : nullptr;
}

const FOutbreakWeaponSlot* UOutbreakWeaponComponent::GetActiveSlot() const
{
	return Weapons.IsValidIndex(ActiveWeaponIndex) ? &Weapons[ActiveWeaponIndex] : nullptr;
}

FOutbreakAmmoAmount* UOutbreakWeaponComponent::FindReserve(const UOutbreakAmmoTypeDefinition* AmmoType)
{
	return AmmoReserves.FindByPredicate(
		[AmmoType](const FOutbreakAmmoAmount& Entry) { return Entry.AmmoType.Get() == AmmoType; });
}

void UOutbreakWeaponComponent::AddReserve(UOutbreakAmmoTypeDefinition* AmmoType, int32 Count)
{
	if (FOutbreakAmmoAmount* Reserve = FindReserve(AmmoType))
	{
		Reserve->Count += Count;
		return;
	}

	FOutbreakAmmoAmount& NewReserve = AmmoReserves.AddDefaulted_GetRef();
	NewReserve.AmmoType = AmmoType;
	NewReserve.Count = Count;
}

void UOutbreakWeaponComponent::BroadcastStateChanged()
{
	// Presentation (Phase 13): this runs on the authority and, through OnRep, on every client.
	const FOutbreakWeaponSlot* Slot = GetActiveSlot();
	UOutbreakWeaponDefinition* Current = Slot ? Slot->Definition.Get() : nullptr;
	const TArray<EOutbreakWeaponAction> Actions = OutbreakWeaponActions::DetectStateActions(
		bLastBroadcastReloading, LastBroadcastWeapon.Get(), bIsReloading, Current);
	bLastBroadcastReloading = bIsReloading;
	LastBroadcastWeapon = Current;
	for (const EOutbreakWeaponAction Action : Actions)
	{
		BroadcastWeaponAction(Action, Current);
	}

	OnWeaponStateChanged.Broadcast(this);
}

void UOutbreakWeaponComponent::BroadcastWeaponAction(EOutbreakWeaponAction Action, UOutbreakWeaponDefinition* Weapon)
{
	if (Action != EOutbreakWeaponAction::Fired && GetOwner())
	{
		OutbreakWeaponEffects::PlayActionSound(this, Weapon, Action, GetOwner()->GetActorLocation());
	}
	OnWeaponAction.Broadcast(this, Action, Weapon);
}

void UOutbreakWeaponComponent::OnRep_WeaponState()
{
	BroadcastStateChanged();
}
