#include "Environment/OutbreakDoor.h"

#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Inventory/OutbreakItemDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "NavAreas/NavArea_Default.h"
#include "NavAreas/NavArea_Null.h"
#include "NavModifierComponent.h"
#include "Net/UnrealNetwork.h"
#include "Stealth/OutbreakNoiseTypes.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "OutbreakDoor"

AOutbreakDoor::AOutbreakDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;

	// Root at the floor in the middle of the doorway; the door is 100 cm wide along Y, hinged at -Y.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(RootComponent);
	Hinge->SetRelativeLocation(FVector(0.0f, -50.0f, 0.0f));
	Hinge->SetMobility(EComponentMobility::Movable);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(Hinge);
	DoorMesh->SetMobility(EComponentMobility::Movable);
	DoorMesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	// The navigation modifier decides whether AI may pass, not the door's shape.
	DoorMesh->SetCanEverAffectNavigation(false);
	// Greybox default: the 100 cm engine cube as a 10 x 100 x 220 cm slab.
	DoorMesh->SetRelativeLocation(FVector(0.0f, 50.0f, 110.0f));
	DoorMesh->SetRelativeScale3D(FVector(0.1f, 1.0f, 2.2f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		DoorMesh->SetStaticMesh(CubeMesh.Object);
	}

	AutoOpenTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("AutoOpenTrigger"));
	AutoOpenTrigger->SetupAttachment(RootComponent);
	AutoOpenTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f));
	AutoOpenTrigger->SetBoxExtent(FVector(120.0f, 60.0f, 110.0f));
	AutoOpenTrigger->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	AutoOpenTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AutoOpenTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	AutoOpenTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AutoOpenTrigger->SetGenerateOverlapEvents(true);
	AutoOpenTrigger->SetCanEverAffectNavigation(false);

	NavModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavModifier"));
	// No component affects navigation, so the modifier uses this box around the doorway.
	NavModifier->FailsafeExtent = FVector(30.0f, 55.0f, 110.0f);

	OpenPrompt = LOCTEXT("Open", "Open door");
	ClosePrompt = LOCTEXT("Close", "Close door");
	LockedPrompt = LOCTEXT("Locked", "Locked");
}

void AOutbreakDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOutbreakDoor, bOpen);
	DOREPLIFETIME(AOutbreakDoor, bLocked);
}

void AOutbreakDoor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ClosedHingeTransform = Hinge->GetRelativeTransform();
	if (HasAuthority())
	{
		bOpen = bStartsOpen;
		bLocked = bStartsLocked;
	}
	OpenAlpha = bOpen ? 1.0f : 0.0f;
	UpdateVisuals();
	UpdateNavigation();

	AutoOpenTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleTriggerOverlap);
}

void AOutbreakDoor::BeginPlay()
{
	Super::BeginPlay();
	// The navigation system registers modifiers at play; refresh the area for the starting state.
	UpdateNavigation();
}

void AOutbreakDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Target = bOpen ? 1.0f : 0.0f;
	OpenAlpha = FMath::FInterpConstantTo(OpenAlpha, Target, DeltaSeconds, 1.0f / FMath::Max(OpenTime, 0.05f));
	UpdateVisuals();
	if (FMath::IsNearlyEqual(OpenAlpha, Target))
	{
		OpenAlpha = Target;
		SetActorTickEnabled(false);
	}
}

void AOutbreakDoor::UpdateVisuals()
{
	if (bSliding)
	{
		Hinge->SetRelativeLocation(ClosedHingeTransform.GetLocation() + SlideOffset * OpenAlpha);
	}
	else
	{
		Hinge->SetRelativeRotation(ClosedHingeTransform.Rotator() + FRotator(0.0f, OpenAngle * OpenAlpha, 0.0f));
	}
}

void AOutbreakDoor::StartMoving()
{
	SetActorTickEnabled(true);
}

bool AOutbreakDoor::BlocksNavigation() const
{
	return bLocked || (!bOpen && !bAIOpens);
}

void AOutbreakDoor::UpdateNavigation()
{
	NavModifier->SetAreaClass(BlocksNavigation() ? UNavArea_Null::StaticClass() : UNavArea_Default::StaticClass());
}

void AOutbreakDoor::SetOpen(bool bNewOpen, AActor* Instigator)
{
	if (bOpen == bNewOpen)
	{
		return;
	}
	bOpen = bNewOpen;
	StartMoving();
	UpdateNavigation();

	if (NoiseLoudness > 0.0f)
	{
		FOutbreakNoiseEvent Noise;
		Noise.Type = EOutbreakNoiseType::Other;
		Noise.Location = GetActorLocation();
		Noise.Loudness = NoiseLoudness;
		Noise.Instigator = Instigator;
		OutbreakNoise::ReportNoise(GetWorld(), Noise);
	}
	if (USoundBase* Sound = bOpen ? OpenSound.Get() : CloseSound.Get())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
	OnDoorStateChanged(bOpen, bLocked);
}

void AOutbreakDoor::OnRep_DoorState()
{
	// Clients: animate towards the new state; sounds follow the open state.
	const bool bShownOpen = OpenAlpha > 0.5f;
	if (bShownOpen != bOpen)
	{
		if (USoundBase* Sound = bOpen ? OpenSound.Get() : CloseSound.Get())
		{
			UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
		}
	}
	StartMoving();
	UpdateNavigation();
	OnDoorStateChanged(bOpen, bLocked);
}

bool AOutbreakDoor::Open(AActor* OpenedBy)
{
	if (!HasAuthority() || bLocked || bOpen)
	{
		return false;
	}
	GetWorldTimerManager().ClearTimer(AutoCloseTimer);
	SetOpen(true, OpenedBy);
	return true;
}

void AOutbreakDoor::Close(AActor* ClosedBy)
{
	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(AutoCloseTimer);
		SetOpen(false, ClosedBy);
	}
}

void AOutbreakDoor::SetLocked(bool bNewLocked)
{
	if (!HasAuthority() || bLocked == bNewLocked)
	{
		return;
	}
	bLocked = bNewLocked;
	UpdateNavigation();
	OnDoorStateChanged(bOpen, bLocked);
}

void AOutbreakDoor::SetKey(UOutbreakItemDefinition* NewKey, bool bConsume)
{
	if (HasAuthority())
	{
		Key = NewKey;
		bConsumeKey = bConsume;
	}
}

void AOutbreakDoor::SetSwitchedOn_Implementation(bool bOn, AActor* SwitchedBy)
{
	if (bOn)
	{
		SetLocked(false);
		Open(SwitchedBy);
	}
	else
	{
		Close(SwitchedBy);
		SetLocked(true);
	}
}

bool AOutbreakDoor::HasKey(const AActor* InteractionInstigator) const
{
	const UOutbreakInventoryComponent* Inventory = Key && InteractionInstigator
		? InteractionInstigator->FindComponentByClass<UOutbreakInventoryComponent>() : nullptr;
	return Inventory && Inventory->GetItemCount(Key) > 0;
}

bool AOutbreakDoor::CanInteract_Implementation(AActor* InteractionInstigator) const
{
	// Always focusable, so a locked door can say why it does not open.
	return true;
}

FText AOutbreakDoor::GetInteractionPrompt_Implementation(AActor* InteractionInstigator) const
{
	if (bLocked)
	{
		if (HasKey(InteractionInstigator))
		{
			return FText::Format(LOCTEXT("Unlock", "Unlock with {0}"), Key->DisplayName);
		}
		return Key ? FText::Format(LOCTEXT("RequiresKey", "Locked: requires {0}"), Key->DisplayName) : LockedPrompt;
	}
	return bOpen ? ClosePrompt : OpenPrompt;
}

void AOutbreakDoor::Interact_Implementation(AActor* InteractionInstigator)
{
	if (!HasAuthority())
	{
		return;
	}
	if (bLocked)
	{
		if (!HasKey(InteractionInstigator))
		{
			if (LockedSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, LockedSound, GetActorLocation());
			}
			return;
		}
		if (bConsumeKey)
		{
			InteractionInstigator->FindComponentByClass<UOutbreakInventoryComponent>()->RemoveItem(Key, 1);
		}
		SetLocked(false);
		Open(InteractionInstigator);
		return;
	}
	if (bOpen)
	{
		Close(InteractionInstigator);
	}
	else
	{
		Open(InteractionInstigator);
	}
}

bool AOutbreakDoor::TryAutoOpen(APawn* Pawn)
{
	// Only AI-controlled pawns: players use Interact, and pawns without a controller (corpses) do nothing.
	if (!HasAuthority() || !bAIOpens || bLocked || bOpen || !Pawn || !Cast<AAIController>(Pawn->GetController()))
	{
		return false;
	}
	Open(Pawn);
	if (AutoCloseDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(AutoCloseTimer, this, &ThisClass::TryAutoClose, AutoCloseDelay, false);
	}
	return true;
}

void AOutbreakDoor::TryAutoClose()
{
	if (!bOpen)
	{
		return;
	}
	TArray<AActor*> InDoorway;
	AutoOpenTrigger->GetOverlappingActors(InDoorway, APawn::StaticClass());
	if (!InDoorway.IsEmpty())
	{
		// Someone is still in the way; try again later.
		GetWorldTimerManager().SetTimer(AutoCloseTimer, this, &ThisClass::TryAutoClose, AutoCloseDelay, false);
		return;
	}
	Close(nullptr);
}

void AOutbreakDoor::HandleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasAuthority())
	{
		TryAutoOpen(Cast<APawn>(OtherActor));
	}
}

#undef LOCTEXT_NAMESPACE
