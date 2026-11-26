#include "Combat/OutbreakDamageApplication.h"

#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Stealth/OutbreakNoiseTypes.h"

namespace OutbreakDamage
{
	void ApplyImpactDamage(const FHitResult& Hit, const FVector& ShotDirection,
		const FOutbreakImpactDamage& ImpactDamage, AController* InstigatorController, AActor* DamageCauser)
	{
		if (ImpactDamage.Damage <= 0.0f || !DamageCauser || !DamageCauser->HasAuthority())
		{
			return;
		}

		const TSubclassOf<UDamageType> DamageType = ImpactDamage.DamageType
			? ImpactDamage.DamageType
			: TSubclassOf<UDamageType>(UDamageType::StaticClass());

		if (ImpactDamage.ExplosionRadius > 0.0f)
		{
			FOutbreakNoiseEvent Noise;
			Noise.Type = EOutbreakNoiseType::Explosion;
			Noise.Location = Hit.ImpactPoint;
			Noise.Loudness = 1.0f;
			Noise.Instigator = DamageCauser->GetInstigator() ? static_cast<AActor*>(DamageCauser->GetInstigator()) : DamageCauser;
			OutbreakNoise::ReportNoise(DamageCauser->GetWorld(), Noise);

			UGameplayStatics::ApplyRadialDamage(DamageCauser, ImpactDamage.Damage, Hit.ImpactPoint,
				ImpactDamage.ExplosionRadius, DamageType, TArray<AActor*>(), DamageCauser, InstigatorController,
				/*bDoFullDamage=*/ false, ECC_Visibility);
			return;
		}

		UGameplayStatics::ApplyPointDamage(Hit.GetActor(), ImpactDamage.Damage, ShotDirection, Hit,
			InstigatorController, DamageCauser, DamageType);
	}
}
