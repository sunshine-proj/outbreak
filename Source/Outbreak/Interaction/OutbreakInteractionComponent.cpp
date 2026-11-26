#include "Interaction/OutbreakInteractionComponent.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Interaction/OutbreakInteractable.h"
#include "Outbreak.h"
#include "TimerManager.h"

UOutbreakInteractionComponent::UOutbreakInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// For the ServerInteract RPC.
	SetIsReplicatedByDefault(true);
}

void UOutbreakInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->GetTimerManager().SetTimer(
		FocusUpdateTimer, this, &ThisClass::UpdateFocus, FocusUpdateInterval, /*bLoop=*/ true);
}

void UOutbreakInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FocusUpdateTimer);
	}

	Super::EndPlay(EndPlayReason);
}

bool UOutbreakInteractionComponent::IsValidInteractionTarget(const AActor* Candidate, float DistanceToCandidate) const
{
	if (!IsValid(Candidate) || !Candidate->Implements<UOutbreakInteractable>())
	{
		return false;
	}

	if (DistanceToCandidate < 0.0f || DistanceToCandidate > InteractionRange)
	{
		return false;
	}

	return IOutbreakInteractable::Execute_CanInteract(Candidate, GetOwner());
}

void UOutbreakInteractionComponent::UpdateFocus()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && !OwnerPawn->IsLocallyControlled())
	{
		SetFocus(nullptr);
		return;
	}

	SetFocus(FindFocusCandidate());
}

AActor* UOutbreakInteractionComponent::FindFocusCandidate() const
{
	const AActor* Owner = GetOwner();
	const UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return nullptr;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Owner->GetActorEyesViewPoint(ViewLocation, ViewRotation);

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * InteractionRange;
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OutbreakInteractionTrace), /*bTraceComplex=*/ false, Owner);

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, TraceChannel, QueryParams))
	{
		return nullptr;
	}

	AActor* HitActor = Hit.GetActor();
	return IsValidInteractionTarget(HitActor, Hit.Distance) ? HitActor : nullptr;
}

void UOutbreakInteractionComponent::SetFocus(AActor* NewFocus)
{
	const FText NewPrompt = NewFocus
		? IOutbreakInteractable::Execute_GetInteractionPrompt(NewFocus, GetOwner())
		: FText::GetEmpty();

	if (NewFocus == FocusedActor.Get() && NewPrompt.EqualTo(FocusedPrompt))
	{
		return;
	}

	FocusedActor = NewFocus;
	FocusedPrompt = NewPrompt;
	OnFocusChanged.Broadcast(NewFocus, FocusedPrompt);
}

bool UOutbreakInteractionComponent::TryInteract()
{
	// Re-trace so the interaction uses the current view and range, not a focus up to one interval old.
	UpdateFocus();

	AActor* Target = FocusedActor.Get();
	if (!Target)
	{
		return false;
	}

	UE_LOG(LogOutbreak, Verbose, TEXT("%s interacts with %s."), *GetNameSafe(GetOwner()), *Target->GetName());
	if (GetOwner()->HasAuthority())
	{
		AuthInteract(Target);
	}
	else
	{
		ServerInteract(Target);
	}

	// The interaction may change the prompt (for example "Open" -> "Close").
	UpdateFocus();
	return true;
}

void UOutbreakInteractionComponent::ServerInteract_Implementation(AActor* Target)
{
	AuthInteract(Target);
}

void UOutbreakInteractionComponent::AuthInteract(AActor* Target)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !IsValid(Target) || !Target->Implements<UOutbreakInteractable>())
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Owner->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	if (FVector::Dist(ViewLocation, Target->GetActorLocation()) > InteractionRange + ServerRangeTolerance
		|| !IOutbreakInteractable::Execute_CanInteract(Target, GetOwner()))
	{
		UE_LOG(LogOutbreak, Verbose, TEXT("%s: interaction with %s rejected (range or state)."), *GetNameSafe(Owner), *Target->GetName());
		return;
	}
	IOutbreakInteractable::Execute_Interact(Target, GetOwner());
}
