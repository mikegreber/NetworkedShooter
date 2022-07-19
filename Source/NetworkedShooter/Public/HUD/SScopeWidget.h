// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SScopeWidget.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API USScopeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetAiming(bool bAiming);
};
