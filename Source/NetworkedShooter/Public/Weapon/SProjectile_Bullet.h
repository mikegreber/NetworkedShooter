// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Weapon/SProjectile.h"
#include "SProjectile_Bullet.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API ASProjectile_Bullet : public ASProjectile
{
	GENERATED_BODY()

public:
	ASProjectile_Bullet();
	
	// virtual void ApplyDamage(const UObject* WorldContextObject, const FVector& Location, AActor* DamagedActor, float BaseDamage, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass, ECollisionChannel DamageChannel = ECC_Visibility) const override;

	virtual void ApplyDamage(const UObject* WorldContextObject, const FVector& Location, AActor* DamagedActor,
		float BaseDamage, IAbilitySystemInterface* Source, AActor* DamageCauser,
		TSubclassOf<UGameplayEffect> DamageEffectClass, ECollisionChannel DamageChannel) const override;

protected:
	
	virtual void BeginPlay() override;

	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
};
