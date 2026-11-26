#include "Game/OutbreakGameState.h"

#include "Net/UnrealNetwork.h"

void AOutbreakGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOutbreakGameState, MissionStatus);
}

void AOutbreakGameState::SetMissionStatus(const FOutbreakMissionStatus& NewStatus)
{
	if (!HasAuthority())
	{
		return;
	}

	const bool bChanged = NewStatus.State != MissionStatus.State
		|| NewStatus.ObjectiveIndex != MissionStatus.ObjectiveIndex
		|| !NewStatus.ObjectiveDescription.EqualTo(MissionStatus.ObjectiveDescription)
		|| !NewStatus.ObjectiveProgress.EqualTo(MissionStatus.ObjectiveProgress)
		|| !NewStatus.ResultText.EqualTo(MissionStatus.ResultText)
		|| !NewStatus.MissionName.EqualTo(MissionStatus.MissionName)
		|| NewStatus.ObjectiveCount != MissionStatus.ObjectiveCount;

	MissionStatus = NewStatus;
	if (bChanged)
	{
		OnMissionStatusChanged.Broadcast(MissionStatus);
	}
}

void AOutbreakGameState::OnRep_MissionStatus()
{
	OnMissionStatusChanged.Broadcast(MissionStatus);
}
