// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/SProjectile.h"
#include "SProjectile_Bullet.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API ASProjectile_Bullet : public ASProjectile
{
	GENERATED_BODY()

protected:

	ASProjectile_Bullet();
	
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
};
