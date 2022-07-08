// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/SWeapon.h"
#include "SWeapon_Projectile.generated.h"

UCLASS()
class NETWORKEDSHOOTER_API ASWeapon_Projectile : public ASWeapon
{
	GENERATED_BODY()

protected:
	
	UPROPERTY(EditAnywhere, Category = "WeaponProperties | Projectile")
	TSubclassOf<class ASProjectile> ProjectileClass;
	
	virtual void LocalFire(const FVector_NetQuantize& HitTarget) override;

};
