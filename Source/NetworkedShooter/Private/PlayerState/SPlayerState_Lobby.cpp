// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/SPlayerState_Lobby.h"

#include "Character/SCharacter_Lobby.h"
#include "GameState/SGameState_Lobby.h"
#include "Net/UnrealNetwork.h"
#include "NetworkedShooter/NetworkedShooter.h"

void ASPlayerState_Lobby::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASPlayerState_Lobby, bIsReady);
}

void ASPlayerState_Lobby::OnRep_IsReady()
{
	UE_LOG(LogTemp, Warning, TEXT("%s %s %d"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR, bIsReady);
	OnIsReadyChanged.Broadcast(bIsReady);
	if (ASCharacter_Lobby* Character = GetPawn<ASCharacter_Lobby>())
	{
		Character->SetBottomOverheadText(bIsReady ? "Ready" : "Not Ready", bIsReady ? FColor::Green : FColor::Red);
	}
}

void ASPlayerState_Lobby::ToggleReady_Implementation()
{
	bIsReady = !bIsReady;
	OnRep_IsReady();
	GetWorld()->GetGameState<ASGameState_Lobby>()->UpdatePlayerState(this);
}
