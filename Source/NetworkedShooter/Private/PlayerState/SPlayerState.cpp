// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/SPlayerState.h"

#include "HUD/SHUD.h"
#include "Net/UnrealNetwork.h"
#include "HUD/SHUD.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "PlayerController/SPlayerController.h"

void ASPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASPlayerState, Kills);
	DOREPLIFETIME(ASPlayerState, Deaths);
	DOREPLIFETIME(ASPlayerState, bHasHighPing);
}

void ASPlayerState::BeginPlay()
{
	Super::BeginPlay();
}

void ASPlayerState::SetOwner(AActor* NewOwner)
{
	Super::SetOwner(NewOwner);
	
	PlayerController = Cast<ASPlayerController>(NewOwner);
}

void ASPlayerState::OnPlayerHUDCreated(ASHUD* HUD)
{
}

void ASPlayerState::AddToKills(int32 KillsAmount)
{
	SERVER_ONLY();
	
	if (HasAuthority())
	{
		Kills += KillsAmount;
		if (PlayerController->HasLocalAuthority()) OnRep_Kills();
	}
}

void ASPlayerState::OnRep_Kills()
{
	UpdateKills();
}

void ASPlayerState::UpdateKills() const
{
	OnKillsUpdated.Broadcast(Kills);
}

void ASPlayerState::AddToDeaths(int32 DefeatsAmount)
{
	SERVER_ONLY();
	
	if (HasAuthority())
	{
		Deaths += DefeatsAmount;
		if (PlayerController->HasLocalAuthority()) OnRep_Deaths();
	}
}

void ASPlayerState::UpdateDeaths()
{
	OnDeathsUpdated.Broadcast(Deaths);
}

void ASPlayerState::OnRep_Deaths()
{
	UpdateDeaths();
}


