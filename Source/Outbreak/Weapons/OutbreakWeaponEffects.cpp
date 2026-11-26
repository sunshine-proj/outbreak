#include "Weapons/OutbreakWeaponEffects.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakWeaponDefinition.h"
#include "Weapons/OutbreakWeaponTypes.h"

namespace OutbreakWeaponEffects
{
	namespace
	{
		bool CanPlayEffects(const UObject* WorldContextObject)
		{
			const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
			return World && World->GetNetMode() != NM_DedicatedServer;
		}
	}

	void PlayActionSound(const UObject* WorldContextObject, const UOutbreakWeaponDefinition* Weapon,
		EOutbreakWeaponAction Action, const FVector& Location)
	{
		if (!Weapon || !CanPlayEffects(WorldContextObject))
		{
			return;
		}
		USoundBase* Sound = nullptr;
		switch (Action)
		{
		case EOutbreakWeaponAction::ReloadStarted: Sound = Weapon->ReloadSound; break;
		case EOutbreakWeaponAction::Equipped:      Sound = Weapon->EquipSound; break;
		case EOutbreakWeaponAction::DryFired:      Sound = Weapon->EmptySound; break;
		default: break;
		}
		if (Sound)
		{
			UGameplayStatics::PlaySoundAtLocation(WorldContextObject, Sound, Location);
		}
	}

	void PlayMuzzleEffects(const UObject* WorldContextObject, const UOutbreakWeaponDefinition* Weapon,
		const FVector& ViewOrigin, const FVector& ViewDirection)
	{
		if (!Weapon || !CanPlayEffects(WorldContextObject))
		{
			return;
		}

		const FRotator ViewRotation = ViewDirection.Rotation();
		const FVector MuzzleLocation = ViewOrigin + ViewRotation.RotateVector(Weapon->MuzzleOffset);

		if (Weapon->MuzzleFlashEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContextObject, Weapon->MuzzleFlashEffect.Get(),
				MuzzleLocation, ViewRotation);
		}
		if (Weapon->FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(WorldContextObject, Weapon->FireSound.Get(), MuzzleLocation);
		}
	}

	void PlayImpactEffects(const UObject* WorldContextObject, const UOutbreakAmmoTypeDefinition* AmmoType,
		const FVector& Location, const FVector& Normal)
	{
		if (!AmmoType || !CanPlayEffects(WorldContextObject))
		{
			return;
		}

		if (AmmoType->ImpactEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContextObject, AmmoType->ImpactEffect.Get(),
				Location, Normal.Rotation());
		}
		if (AmmoType->ImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(WorldContextObject, AmmoType->ImpactSound.Get(), Location);
		}
	}
}
