// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/SGameMode_Lobby.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

void ASGameMode_Lobby::StartGame()
{
	if (UWorld* World = GetWorld())
	{
		bUseSeamlessTravel = true;
		World->ServerTravel(FString("/Game/Maps/ShooterMap?listen"));
	}
}

void ASGameMode_Lobby::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (GameState)
	{
		const int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
		// if (NumberOfPlayers == 2)
		// {
		// 	StartGame();
		// }
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				1,
				60.f,
				FColor::Yellow,
				FString::Printf(TEXT("Players in game: %d"), NumberOfPlayers)
				);

			if (const APlayerState* PS = NewPlayer->GetPlayerState<APlayerState>())
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					60.f,
					FColor::Cyan,
					FString::Printf(TEXT("%s has joined the game!"), *PS->GetPlayerName())
					);
			}
		}
	}
}

void ASGameMode_Lobby::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (const APlayerState* PS = Exiting->GetPlayerState<APlayerState>())
	{
		const int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				1,
				60.f,
				FColor::Yellow,
				FString::Printf(TEXT("Players in game: %d"), NumberOfPlayers - 1)
				);
			
			GEngine->AddOnScreenDebugMessage(
				-1,
				60.f,
				FColor::Cyan,
				FString::Printf(TEXT("%s has exited the game!"), *PS->GetPlayerName()));
		}
	}
}