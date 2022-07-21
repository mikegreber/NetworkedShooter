// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API USCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	UPROPERTY() class USAttributeSet* AttributeSet;

	class FSSavedMove : public FSavedMove_Character
	{
	public:
		typedef FSavedMove_Character Super;

		virtual void Clear() override;

		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;
		
		virtual void PrepMoveFor(ACharacter* Character) override;
		
		virtual uint8 GetCompressedFlags() const override;

		uint8 SavedRequestToStartSprinting : 1;

		uint8 SavedRequestToStartADS : 1;
	};

	class FSNetworkPredictionData_Client : public FNetworkPredictionData_Client_Character
	{
	public:
		FSNetworkPredictionData_Client(const UCharacterMovementComponent& ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		virtual FSavedMovePtr AllocateNewMove() override;
	};
	
public:
	USCharacterMovementComponent();

	void SetAttributeSet(USAttributeSet* NewAttributeSet) { AttributeSet = NewAttributeSet; }
	
	UPROPERTY(EditAnywhere, Category = "Settings")
	float SprintSpeed;
	
	UPROPERTY(EditAnywhere, Category = "Settings")
	float ADSSpeed;

	uint8 RequestToStartSprinting : 1;
	uint8 RequestToStartADS : 1;

	virtual float GetMaxSpeed() const override;
	
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	UFUNCTION(BlueprintCallable, Category = "Sprint")
	void StartSprinting();

	UFUNCTION(BlueprintCallable, Category = "Sprint")
	void StopSprinting();

	UFUNCTION(BlueprintCallable, Category = "Sprint")
	void StartADS();

	UFUNCTION(BlueprintCallable, Category = "Sprint")
	void StopADS();
	
	
};
