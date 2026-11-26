#include "Environment/OutbreakEnvironmentVolume.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/Pawn.h"
#include "Sound/SoundBase.h"

AOutbreakEnvironmentVolume::AOutbreakEnvironmentVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetBoxExtent(FVector(1000.0f, 1000.0f, 400.0f));
	// Overlaps only, for the ambience; perception queries use ContainsPoint.
	Bounds->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Bounds->SetCanEverAffectNavigation(false);
	Bounds->SetHiddenInGame(true);
	RootComponent = Bounds;

	Ambience = CreateDefaultSubobject<UAudioComponent>(TEXT("Ambience"));
	Ambience->SetupAttachment(Bounds);
	Ambience->bAutoActivate = false;
	Ambience->bAllowSpatialization = false;

#if WITH_EDITORONLY_DATA
	// Perception queries need every zone loaded.
	bIsSpatiallyLoaded = false;
#endif
}

void AOutbreakEnvironmentVolume::BeginPlay()
{
	Super::BeginPlay();

	// Only zones with ambience need overlap events, and only pawns matter.
	if (AmbientSound && GetNetMode() != NM_DedicatedServer)
	{
		Ambience->SetSound(AmbientSound);
		Bounds->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
		Bounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Bounds->SetCollisionResponseToAllChannels(ECR_Ignore);
		Bounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Bounds->SetGenerateOverlapEvents(true);
		Bounds->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleBeginOverlap);
		Bounds->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::HandleEndOverlap);
		// A player already standing inside gets the ambience without having to move first.
		Bounds->UpdateOverlaps();
	}
}

bool AOutbreakEnvironmentVolume::ContainsPoint(const FVector& Point) const
{
	const FVector Local = Bounds->GetComponentTransform().InverseTransformPosition(Point);
	const FVector Extent = Bounds->GetUnscaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}

bool AOutbreakEnvironmentVolume::IsLocalPlayerPawn(const AActor* Actor)
{
	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn && Pawn->IsLocallyControlled() && Pawn->IsPlayerControlled();
}

void AOutbreakEnvironmentVolume::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsLocalPlayerPawn(OtherActor))
	{
		Ambience->FadeIn(AmbienceFadeTime);
	}
}

void AOutbreakEnvironmentVolume::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (IsLocalPlayerPawn(OtherActor))
	{
		Ambience->FadeOut(AmbienceFadeTime, 0.0f);
	}
}
