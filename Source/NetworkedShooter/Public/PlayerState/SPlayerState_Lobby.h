// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SPlayerState_Lobby.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIsReadyChanged, bool, bReady);
/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API ASPlayerState_Lobby : public APlayerState
{
	GENERATED_BODY()

	friend class ASPlayerController_Lobby;
	
	UPROPERTY(ReplicatedUsing=OnRep_IsReady)
	bool bIsReady;

public:
	FOnIsReadyChanged OnIsReadyChanged;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(Server, Reliable)
	void ToggleReady();

	void SetIsReady(bool bReady) { bIsReady = bReady; }

	UFUNCTION()
	void OnRep_IsReady();
	
	FORCEINLINE bool IsReady() const { return bIsReady; }
};
