#include "Squad/OutbreakTeammateCharacter.h"

#include "AI/OutbreakSquadAIController.h"
#include "AI/OutbreakTeammateAIConfig.h"
#include "GameFramework/CharacterMovementComponent.h"

AOutbreakTeammateCharacter::AOutbreakTeammateCharacter()
{
	AIControllerClass = AOutbreakSquadAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	Team = EOutbreakTeam::Player;

	// AI aims with control rotation; smooth turning reads better than snapping to the focus.
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	bUseControllerRotationYaw = false;
	Movement->bUseControllerDesiredRotation = true;
	Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
}
