// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/SGameMode_Lobby.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "PlayerController/SPlayerController_Lobby.h"

void ASGameMode_Lobby::StartGame()
{
	if (UWorld* World = GetWorld())
	{
		const FString URLString = FString::Printf(TEXT("%s?listen?game=%s"), *SelectedMap.PrimaryAssetName.ToString(), *SelectedGameMode->GetPathName());
		UE_LOG(LogTemp, Warning, TEXT("%s %s"), __FUNCTIONW__, *URLString)
		bUseSeamlessTravel = !WITH_EDITOR;
		
		World->ServerTravel(URLString);
	}
}

void ASGameMode_Lobby::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// if (GameState)
	// {
	// 	const int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
	// 	
	// 	if (GEngine)
	// 	{
	// 		GEngine->AddOnScreenDebugMessage(
	// 			1,
	// 			5.f,
	// 			FColor::Yellow,
	// 			FString::Printf(TEXT("Players in game: %d"), NumberOfPlayers)
	// 			);
	// 	
	// 		if (const APlayerState* PS = NewPlayer->GetPlayerState<APlayerState>())
	// 		{
	// 			GEngine->AddOnScreenDebugMessage(
	// 				-1,
	// 				5.f,
	// 				FColor::Cyan,
	// 				FString::Printf(TEXT("%s has joined the game!"), *PS->GetPlayerName())
	// 				);
	// 		}
	// 	}
	// }
}

void ASGameMode_Lobby::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (const APlayerState* PS = Exiting->GetPlayerState<APlayerState>())
	{
		const int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

		// if (GEngine)
		// {
		// 	GEngine->AddOnScreenDebugMessage(
		// 		1,
		// 		5.f,
		// 		FColor::Yellow,
		// 		FString::Printf(TEXT("Players in game: %d"), NumberOfPlayers - 1)
		// 		);
		// 	
		// 	GEngine->AddOnScreenDebugMessage(
		// 		-1,
		// 		5.f,
		// 		FColor::Cyan,
		// 		FString::Printf(TEXT("%s has exited the game!"), *PS->GetPlayerName()));
		// }
	}
}

void ASGameMode_Lobby::NotifyGameModeSelectionChanged(int32 SelectedIndex)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASPlayerController_Lobby* PlayerController = Cast<ASPlayerController_Lobby>(*It))
		{
			PlayerController->ClientNotifyGameModeSelectionChanged(SelectedIndex);
		}
	}
}

void ASGameMode_Lobby::NotifyMapSelectionChanged(int32 SelectedIndex)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASPlayerController_Lobby* PlayerController = Cast<ASPlayerController_Lobby>(*It))
		{
			PlayerController->ClientNotifyMapSelectionChanged(SelectedIndex);
		}
	}
}
