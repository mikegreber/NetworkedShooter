// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SCharacter_Lobby.generated.h"

UCLASS()
class NETWORKEDSHOOTER_API ASCharacter_Lobby : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* ViewCamera;

	UPROPERTY(EditAnywhere, Category = "Settings")
	TSubclassOf<AActor> CameraViewActorClass;

    UPROPERTY(VisibleAnywhere, Category = Components)
    class UWidgetComponent* TopOverheadWidget;

	UPROPERTY(VisibleAnywhere, Category = Components)
	class UWidgetComponent* MiddleOverheadWidget;
	
	UPROPERTY(VisibleAnywhere, Category = Components)
	class UWidgetComponent* BottomOverheadWidget;

	UPROPERTY() class ASPlayerState_Lobby* LobbyPlayerState;
	
public:
	ASCharacter_Lobby();
	
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	void SetTopOverheadText(FString Text, FColor Color = FColor(255,255,255));
	void SetMiddleOverheadText(FString Text, FColor Color = FColor(255,255,255));
	void SetBottomOverheadText(FString Text, FColor Color = FColor(255,255,255));
	

	
protected:
	virtual void BeginPlay() override;

private:
	void SetLobbyPlayerState(APlayerState* PlayerState);

};
