#include "Squad/OutbreakTeammateCharacter.h"

#include "AIController.h"

AOutbreakTeammateCharacter::AOutbreakTeammateCharacter()
{
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	Team = EOutbreakTeam::Player;
}
