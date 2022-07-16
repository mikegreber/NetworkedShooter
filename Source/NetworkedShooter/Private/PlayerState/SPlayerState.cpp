// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/SPlayerState.h"

#include "Character/SCharacter.h"
#include "HUD/SHUD.h"
#include "Net/UnrealNetwork.h"
#include "HUD/SHUD.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "PlayerController/SPlayerController.h"

void ASPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASPlayerState, Team);
	DOREPLIFETIME(ASPlayerState, Kills);
	DOREPLIFETIME(ASPlayerState, Deaths);
	DOREPLIFETIME(ASPlayerState, bHasHighPing);
}

void ASPlayerState::SetTeam(ETeam NewTeam)
{
	Team = NewTeam;
	OnRep_Team();
}

void ASPlayerState::BeginPlay()
{
	Super::BeginPlay();
}

void ASPlayerState::OnRep_Team()
{
	if (ASCharacter* Character = GetPawn<ASCharacter>())
	{
		Character->SetTeamColor(Team);
	}
}

void ASPlayerState::SetOwner(AActor* NewOwner)
{
	Super::SetOwner(NewOwner);
	
	PlayerController = Cast<ASPlayerController>(NewOwner);
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


