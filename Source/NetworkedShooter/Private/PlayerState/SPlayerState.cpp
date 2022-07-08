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
}

void ASPlayerState::BeginPlay()
{
	Super::BeginPlay();
}

void ASPlayerState::SetOwner(AActor* NewOwner)
{
	Super::SetOwner(NewOwner);
	
	Controller = Cast<ASPlayerController>(NewOwner);
	// if (!Controller)
	// {
	// 	Controller = Cast<ASPlayerController>(NewOwner);
	// 	if (Controller->IsLocalController())
	// 	{
	// 		if (Controller->GetHUD() && Controller->HasActorBegunPlay())
	// 		{
	// 			UE_LOG(LogTemp, Warning, TEXT("%s default"), __FUNCTIONW__)
	// 			OnPlayerHUDCreated(Cast<ASHUD>(Controller->GetHUD()));
	// 		}
	// 		else
	// 		{
	// 			UE_LOG(LogTemp, Warning, TEXT("%s set delegate"), __FUNCTIONW__)
	// 			Controller->OnHUDWidgetsCreated.AddDynamic(this, &ASPlayerState::OnPlayerHUDCreated);
	// 		}
	// 	}
	// }
	
}

void ASPlayerState::OnPlayerHUDCreated(ASHUD* HUD)
{
	// OnKillsUpdated.AddDynamic(HUD, &ASHUD::UpdateKills);
	// OnKillsUpdated.Broadcast(Kills);
	//
	// OnDeathsUpdated.AddDynamic(HUD, &ASHUD::ASHUD::UpdateDeaths);
	// OnDeathsUpdated.Broadcast(Deaths);
	//
	// UE_LOG(LogTemp, Warning, TEXT("%s"), __FUNCTIONW__)
	// Controller->OnHUDWidgetsCreated.RemoveDynamic(this, &ASPlayerState::OnPlayerHUDCreated);
}

// void ASPlayerState::UpdateHUD() const
// {
// 	if (Controller)
// 	{
// 		Controller->SetHUDKills(Kills);
// 		Controller->SetHUDDeaths(Deaths);
// 	}
// }

void ASPlayerState::AddToKills(int32 KillsAmount)
{
	SERVER_ONLY();
	
	if (HasAuthority())
	{
		Kills += KillsAmount;
		if (Controller->HasLocalAuthority()) OnRep_Kills();
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
		if (Controller->HasLocalAuthority()) OnRep_Deaths();
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


