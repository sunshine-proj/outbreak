#include "Player/OutbreakCheatManager.h"

#include "AI/OutbreakEnemyAIController.h"
#include "AI/OutbreakEnemyDefinition.h"
#include "Characters/OutbreakEnemyCharacter.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Outbreak.h"
#include "Squad/OutbreakTeammateCharacter.h"

void UOutbreakCheatManager::OutbreakSpawnEnemies(int32 Count, const FString& DefinitionPath, const FString& ClassPath, float Radius)
{
	UWorld* World = GetWorld();
	const APlayerController* Player = GetPlayerController();
	const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	if (!World || !Pawn || Count <= 0)
	{
		return;
	}

	UOutbreakEnemyDefinition* Definition = nullptr;
	if (!DefinitionPath.IsEmpty())
	{
		Definition = LoadObject<UOutbreakEnemyDefinition>(nullptr, *DefinitionPath);
		if (!Definition)
		{
			UE_LOG(LogOutbreak, Warning, TEXT("OutbreakSpawnEnemies: no enemy definition at %s."), *DefinitionPath);
			return;
		}
	}
	UClass* EnemyClass = AOutbreakEnemyCharacter::StaticClass();
	if (!ClassPath.IsEmpty())
	{
		EnemyClass = LoadClass<AOutbreakEnemyCharacter>(nullptr, *ClassPath);
		if (!EnemyClass)
		{
			UE_LOG(LogOutbreak, Warning, TEXT("OutbreakSpawnEnemies: no enemy class at %s (use the _C class path)."), *ClassPath);
			return;
		}
	}

	int32 Spawned = 0;
	const FVector Center = Pawn->GetActorLocation();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Angle = 2.0f * PI * Index / Count;
		const FVector Location = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * FMath::Max(100.0f, Radius);
		const FRotator Facing = (Center - Location).Rotation();
		const FTransform Transform(FRotator(0.0f, Facing.Yaw, 0.0f), Location);
		AOutbreakEnemyCharacter* Enemy = World->SpawnActorDeferred<AOutbreakEnemyCharacter>(EnemyClass, Transform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Enemy)
		{
			continue;
		}
		if (Definition)
		{
			Enemy->SetEnemyDefinition(Definition);
		}
		Enemy->FinishSpawning(Transform);
		++Spawned;
	}
	UE_LOG(LogOutbreak, Log, TEXT("OutbreakSpawnEnemies: spawned %d of %d around %s."), Spawned, Count, *Center.ToString());
}

void UOutbreakCheatManager::OutbreakAlertEnemies()
{
	const APlayerController* Player = GetPlayerController();
	const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	int32 Alerted = 0;
	for (TActorIterator<AOutbreakEnemyCharacter> It(GetWorld()); It; ++It)
	{
		AOutbreakEnemyAIController* Controller = Cast<AOutbreakEnemyAIController>(It->GetController());
		if (Controller && !It->IsDead())
		{
			Controller->ReportAlert(Pawn->GetActorLocation());
			++Alerted;
		}
	}
	UE_LOG(LogOutbreak, Log, TEXT("OutbreakAlertEnemies: alerted %d enemies."), Alerted);
}

void UOutbreakCheatManager::OutbreakKillEnemies()
{
	int32 Killed = 0;
	for (TActorIterator<AOutbreakEnemyCharacter> It(GetWorld()); It; ++It)
	{
		UOutbreakHealthComponent* Health = It->FindComponentByClass<UOutbreakHealthComponent>();
		if (Health && !It->IsDead())
		{
			// Twice: the first hit may only incapacitate when the definition allows it.
			Health->ApplyDamage(1.0e7f, nullptr);
			Health->ApplyDamage(1.0e7f, nullptr);
			++Killed;
		}
	}
	UE_LOG(LogOutbreak, Log, TEXT("OutbreakKillEnemies: killed %d enemies."), Killed);
}

void UOutbreakCheatManager::OutbreakPerfReport()
{
	TMap<FString, int32> EnemiesByState;
	int32 Enemies = 0;
	for (TActorIterator<AOutbreakEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsDead())
		{
			continue;
		}
		++Enemies;
		EnemiesByState.FindOrAdd(UEnum::GetValueAsString(It->GetEnemyState()))++;
	}
	int32 Teammates = 0;
	for (TActorIterator<AOutbreakTeammateCharacter> It(GetWorld()); It; ++It)
	{
		++Teammates;
	}
	FString States;
	for (const TPair<FString, int32>& Entry : EnemiesByState)
	{
		States += FString::Printf(TEXT(" %s=%d"), *Entry.Key, Entry.Value);
	}
	UE_LOG(LogOutbreak, Display, TEXT("OutbreakPerfReport: %d living enemies (%s ), %d teammates, %d players."),
		Enemies, *States, Teammates, GetWorld()->GetNumPlayerControllers());
}
