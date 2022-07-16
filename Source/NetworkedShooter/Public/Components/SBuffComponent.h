// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Pickups/SPickup_Jump.h"
#include "Pickups/SPickup_Shield.h"
#include "SBuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class NETWORKEDSHOOTER_API USBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USBuffComponent();
	// friend class ASCharacter;

	void SetOwnerCharacter(class ASCharacter* AsCharacter);
	
	void Heal(float HealAmount, float HealingTime);

	void ReplenishShield(float ShieldAmount, float ShieldReplenishTime);
	
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);

	void BuffJump(float JumpZVelocityBuff, float JumpBuffTime);

protected:

	void HealRampUp(float DeltaTime);
	
	void ShieldRampUp(float DeltaTime);

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	UPROPERTY()
	ASCharacter* Character;

	bool bHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;

	bool bReplenishingShield = false;
	float ShieldReplenishRate = 0.f;
	float AmountToReplenishShield = 0.f;

	FTimerHandle SpeedBuffTimer;
	float InitialBaseSpeed;
	float InitialCrouchSpeed;
	void ResetSpeeds();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	FTimerHandle JumpBuffTimer;
	void ResetJump();
	float InitialJumpZVelocity;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpZVelocityBuff);

};

