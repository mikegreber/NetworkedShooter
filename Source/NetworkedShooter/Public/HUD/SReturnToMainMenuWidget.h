// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SReturnToMainMenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API USReturnToMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
	UPROPERTY(meta = (BindWidget))
	class UButton* ReturnButton;
	
public:
	
	void MenuSetup();
	
	void MenuTeardown();

	virtual bool Initialize() override;

protected:

	UFUNCTION()
	virtual void OnDestroySession(bool bWasSuccessful);

	UFUNCTION()
	void OnPlayerLeftGame();
	
private:

	UFUNCTION()
	void ReturnButtonClicked();

	UPROPERTY()
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
};
