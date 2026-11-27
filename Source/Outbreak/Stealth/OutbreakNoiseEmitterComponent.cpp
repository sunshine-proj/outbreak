#include "Stealth/OutbreakNoiseEmitterComponent.h"

#include "Characters/OutbreakCharacter.h"
#include "Environment/OutbreakEnvironmentSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Outbreak.h"

// Phase 15 profiling: `stat Outbreak`, Insights and the Outbreak CSV category.
DECLARE_CYCLE_STAT(TEXT("Noise event"), STAT_OutbreakNoise, STATGROUP_Outbreak);

UOutbreakNoiseEmitterComponent::UOutbreakNoiseEmitterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UOutbreakNoiseEmitterComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority() && Cast<ACharacter>(GetOwner()))
	{
		GetWorld()->GetTimerManager().SetTimer(FootstepTimer, this, &ThisClass::EmitFootstep, MovementNoise.FootstepInterval, true);
	}
}

void UOutbreakNoiseEmitterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FootstepTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UOutbreakNoiseEmitterComponent::ReportNoise(EOutbreakNoiseType Type, float Loudness, float RangeOverride)
{
	OUTBREAK_SCOPE_CYCLE_COUNTER(STAT_OutbreakNoise);

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	FOutbreakNoiseEvent Noise;
	Noise.Type = Type;
	Noise.Location = Owner->GetActorLocation();
	Noise.Loudness = Loudness;
	Noise.RangeOverride = RangeOverride;
	Noise.Instigator = Owner;
	OutbreakNoise::ReportNoise(GetWorld(), Noise);
}

float UOutbreakNoiseEmitterComponent::GetCurrentMovementLoudness() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return 0.0f;
	}

	const AOutbreakCharacter* OutbreakCharacter = Cast<AOutbreakCharacter>(Character);
	const bool bSprinting = OutbreakCharacter && OutbreakCharacter->IsSprinting();
	return OutbreakNoise::ComputeMovementLoudness(Character->GetVelocity().Size2D(), Character->bIsCrouched, bSprinting,
		Movement->IsFalling(), MovementNoise, LoudnessMultiplier);
}

void UOutbreakNoiseEmitterComponent::EmitFootstep()
{
	float Loudness = GetCurrentMovementLoudness();
	// Environment (Phase 12): the surface underfoot makes steps louder or quieter.
	UOutbreakEnvironmentSubsystem* Environment = GetWorld() ? GetWorld()->GetSubsystem<UOutbreakEnvironmentSubsystem>() : nullptr;
	if (Environment && GetOwner())
	{
		Loudness *= Environment->GetPerceptionAt(GetOwner()->GetActorLocation()).FootstepLoudnessMultiplier;
	}
	if (Loudness > 0.0f)
	{
		ReportNoise(EOutbreakNoiseType::Footstep, Loudness);
	}
}
