// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/SProjectile.h"
#include "NiagaraComponent.h"
#include "SProjectile_Rocket.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API ASProjectile_Rocket : public ASProjectile
{
	GENERATED_BODY()

public:
	
	ASProjectile_Rocket();

	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;
	
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop;

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation;
	
};
