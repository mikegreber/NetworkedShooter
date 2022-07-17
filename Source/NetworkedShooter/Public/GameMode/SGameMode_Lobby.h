// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SGameMode.h"
#include "GameFramework/GameMode.h"
#include "SGameMode_Lobby.generated.h"

UCLASS()
class NETWORKEDSHOOTER_API ASGameMode_Lobby : public AGameModeBase
{
	GENERATED_BODY()

public:
	void StartGame();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	
	void NotifyGameModeSelectionChanged(int32 SelectedIndex);
	void NotifyMapSelectionChanged(int32 SelectedIndex);

	
	UPROPERTY(EditAnywhere, Category="Map")
	FPrimaryAssetId SelectedMap;
	TSubclassOf<ASGameMode> SelectedGameMode;
};
