// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SProjectile.generated.h"

UCLASS()
class NETWORKEDSHOOTER_API ASProjectile : public AActor
{
	GENERATED_BODY()
	
public:
	
	ASProjectile();

	virtual void Tick(float DeltaTime) override;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;
	
protected:
	
	virtual void BeginPlay() override;

	void StartDestroyTimer();
	void DestroyTimerFinished();
	
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	void SpawnTrailSystem();

	void ExplodeDamage();
	
	UPROPERTY(EditAnywhere, Category = "Damage")
	float Damage;

	UPROPERTY(EditAnywhere, Category = "Damage")
	float DamageInnerRadius = 200.f;
	
	UPROPERTY(EditAnywhere, Category = "Damage")
	float DamageOuterRadius = 500.f;
	
	UPROPERTY(EditAnywhere, Category = "Effects")
	class UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere, Category = "Effects")
	class USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere, Category = "Components")
	class UBoxComponent* CollisionBox;

	UPROPERTY(EditAnywhere, Category = "Effects")
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UProjectileMovementComponent* ProjectileMovementComponent;
	
private:
	
	UPROPERTY(VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UParticleSystemComponent* TracerComponent;
	
	UPROPERTY(EditAnywhere, Category = "Effects")
	class UParticleSystem* Tracer;

	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;
};
