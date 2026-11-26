#include "Game/OutbreakGameState.h"

#include "Missions/OutbreakMissionDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Squad/OutbreakSquadConfiguration.h"

void AOutbreakGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOutbreakGameState, MissionStatus);
	DOREPLIFETIME(AOutbreakGameState, MissionResults);
	DOREPLIFETIME(AOutbreakGameState, MissionDefinition);
	DOREPLIFETIME(AOutbreakGameState, SquadConfiguration);
}

void AOutbreakGameState::SetMissionStatus(const FOutbreakMissionStatus& NewStatus)
{
	if (!HasAuthority())
	{
		return;
	}

	const bool bChanged = !NewStatus.Identical(MissionStatus);
	MissionStatus = NewStatus;
	if (bChanged)
	{
		OnMissionStatusChanged.Broadcast(MissionStatus);
	}
}

void AOutbreakGameState::SetMissionResults(const FOutbreakMissionResults& NewResults)
{
	if (HasAuthority())
	{
		MissionResults = NewResults;
	}
}

void AOutbreakGameState::SetMissionSetup(UOutbreakMissionDefinition* InMissionDefinition, UOutbreakSquadConfiguration* InSquadConfiguration)
{
	if (HasAuthority())
	{
		MissionDefinition = InMissionDefinition;
		SquadConfiguration = InSquadConfiguration;
	}
}

void AOutbreakGameState::OnRep_MissionStatus()
{
	OnMissionStatusChanged.Broadcast(MissionStatus);
}
