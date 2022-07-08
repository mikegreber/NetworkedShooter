// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SRocketMovementComponent.h"

UProjectileMovementComponent::EHandleBlockingHitResult USRocketMovementComponent::HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);
	return EHandleBlockingHitResult::AdvanceNextSubstep;
}

void USRocketMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	// rockets should not stop, only explode when their collision box detects a hit
}
