// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SBuffComponent.h"

#include "Character/SCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

USBuffComponent::USBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed)
{
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void USBuffComponent::SetInitialJumpVelocity(float Velocity)
{
	InitialJumpZVelocity = Velocity;
}


void USBuffComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}

void USBuffComponent::ReplenishShield(float ShieldAmount, float ShieldReplenishTime)
{
	bReplenishingShield = true;
	ShieldReplenishRate = ShieldAmount / ShieldReplenishTime;
	AmountToReplenishShield = ShieldAmount;
}

void USBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime)
{
	if (Character)
	{
		Character->GetWorldTimerManager().SetTimer(
			SpeedBuffTimer,
			this,
			&USBuffComponent::ResetSpeeds,
			BuffTime
		);

		if (Character->GetCharacterMovement())
		{
			Character->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
			Character->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
		}
		MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed);
	}
	
}

void USBuffComponent::ResetSpeeds()
{
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;
	}
	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);

}

void USBuffComponent::BuffJump(float JumpZVelocityBuff, float JumpBuffTime)
{
	if (Character)
	{
		Character->GetWorldTimerManager().SetTimer(
			JumpBuffTimer,
			this,
			&USBuffComponent::ResetJump,
			JumpBuffTime
		);

		if (Character->GetCharacterMovement())
		{
			Character->GetCharacterMovement()->JumpZVelocity = JumpZVelocityBuff;
		}
		MulticastJumpBuff(JumpZVelocityBuff);
	}
}

void USBuffComponent::ResetJump()
{
	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = InitialJumpZVelocity;
	}
	MulticastJumpBuff(InitialJumpZVelocity);
}

void USBuffComponent::MulticastJumpBuff_Implementation(float JumpZVelocityBuff)
{
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = JumpZVelocityBuff;
	}
}

void USBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	}
}

void USBuffComponent::HealRampUp(float DeltaTime)
{
	if (bHealing && Character && !Character->IsEliminated())
	{
		const float HealThisFrame = HealingRate * DeltaTime;
		Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0, Character->GetMaxHealth()));
		AmountToHeal -= HealThisFrame;

		if (AmountToHeal <= 0 || Character->GetHealth() >= Character->GetMaxHealth())
		{
			bHealing = false;
			AmountToHeal = 0;
		}
	}
}

void USBuffComponent::ShieldRampUp(float DeltaTime)
{
	if (bReplenishingShield && Character && !Character->IsEliminated())
	{
		const float ShieldThisFrame = ShieldReplenishRate * DeltaTime;
		Character->SetShield(FMath::Clamp(Character->GetShield() + ShieldThisFrame, 0, Character->GetMaxShield()));
		AmountToHeal -= ShieldThisFrame;

		if (AmountToReplenishShield <= 0 || Character->GetShield() >= Character->GetMaxShield())
		{
			bReplenishingShield = false;
			AmountToReplenishShield = 0;
		}
	}
}


void USBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime);

	ShieldRampUp(DeltaTime);
}

