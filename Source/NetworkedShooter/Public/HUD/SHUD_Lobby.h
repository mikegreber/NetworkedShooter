// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SOverlay_Lobby.h"
#include "GameFramework/HUD.h"
#include "SHUD_Lobby.generated.h"


USTRUCT(BlueprintType)
struct FMapInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FPrimaryAssetId Map;

	UPROPERTY(EditAnywhere)
	FString DisplayName;

	UPROPERTY(EditAnywhere)
	UTexture2D* Thumbnail;	
};

UCLASS()
class NETWORKEDSHOOTER_API ASHUD_Lobby : public AHUD
{
	GENERATED_BODY()

	friend class ASPlayerController_Lobby;

	UPROPERTY(EditAnywhere, Category = "HUD")
	TArray<TSubclassOf<class ASGameMode>> GameModeOptions;

	UPROPERTY(EditAnywhere, Category = "HUD")
	TArray<FMapInfo> MapOptions;
	
	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<USOverlay_Lobby> LobbyOverlayClass;
	UPROPERTY() USOverlay_Lobby* Overlay;

	UPROPERTY() class UButton* ReadyButton;
	UPROPERTY() class UTextBlock* ReadyText;
	UPROPERTY() class UTextBlock* PlayersReadyText;
	UPROPERTY() class UTextBlock* CountdownText;
	UPROPERTY() class UComboBoxString* GameModeComboBox;
	UPROPERTY() class UComboBoxString* MapComboBox;
	UPROPERTY() class UImage* MapThumbnail;
	
protected:
	
	bool Initialize();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
