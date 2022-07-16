// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/SPlayerController_Lobby.h"

#include "GameMode/SGameMode_Lobby.h"

void ASPlayerController_Lobby::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent) return;
	
	InputComponent->BindAction("StartGame", IE_Pressed, this, &ASPlayerController_Lobby::StartGame);
}

void ASPlayerController_Lobby::StartGame()
{
	if (ASGameMode_Lobby* GameMode = GetWorld()->GetAuthGameMode<ASGameMode_Lobby>())
	{
		GameMode->StartGame();
	}
}
