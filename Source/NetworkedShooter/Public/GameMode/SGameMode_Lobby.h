// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SGameMode_Lobby.generated.h"

UCLASS()
class NETWORKEDSHOOTER_API ASGameMode_Lobby : public AGameMode
{
	GENERATED_BODY()

public:
	void StartGame();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	
};
