// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SPlayerController_Lobby.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API ASPlayerController_Lobby : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void SetupInputComponent() override;

	void StartGame();

};
