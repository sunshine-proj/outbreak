#pragma once

#include "CoreMinimal.h"
#include "Combat/OutbreakHealthComponent.h"
#include "GameFramework/Actor.h"

#include "OutbreakDestructibleObject.generated.h"

class UStaticMeshComponent;
class UStaticMesh;

/**
 * A damageable level object for Destroy Object objectives: a relay, a fuel tank, an alien nest.
 *
 * Takes damage through the engine pipeline like any actor with a health component. Its health
 * component's Damage Multipliers decide what can hurt it (for example only explosives). When
 * destroyed it switches to DestroyedMesh, or hides and stops colliding when there is none. It has
 * no team, so AI does not target it on its own.
 */
UCLASS(Blueprintable)
class OUTBREAK_API AOutbreakDestructibleObject : public AActor
{
	GENERATED_BODY()

public:

	AOutbreakDestructibleObject();

	UOutbreakHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "Destructible")
	bool IsDestroyed() const;

protected:

	virtual void PostInitializeComponents() override;

	/** Called on every machine when the object is destroyed, for effects. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Destructible")
	void OnObjectDestroyed();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UOutbreakHealthComponent> HealthComponent;

	/** Shown once destroyed (wreckage). Empty = the object disappears. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destructible")
	TObjectPtr<UStaticMesh> DestroyedMesh;

private:

	UFUNCTION()
	void HandleHealthStateChanged(UOutbreakHealthComponent* InHealthComponent, EOutbreakHealthState NewState, EOutbreakHealthState OldState);
};
