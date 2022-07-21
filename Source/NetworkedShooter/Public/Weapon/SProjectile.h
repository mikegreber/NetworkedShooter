// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "SProjectile.generated.h"

UCLASS(Abstract)
class NETWORKEDSHOOTER_API ASProjectile : public AActor
{
	GENERATED_BODY()

public:
	// should set when projectile is spawned for weapon fired projectiles
	UPROPERTY(EditAnywhere, Category = "Damage")
	TSubclassOf<class UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditAnywhere, Category = "Damage")
    float Damage = 20.f;

    UPROPERTY(EditAnywhere, Category = "Damage")
    float HeadshotDamage = 30.f;
    	
	UPROPERTY(EditAnywhere, Category = "Properties | Projectile")
	float InitialSpeed = 150000.f;

	UPROPERTY(EditAnywhere, Category = "Properties | Projectile")
	float ProjectileGravityScale = 1.f;

protected:
	
	UPROPERTY(EditAnywhere, Category = "Properties | Damage")
	float DamageInnerRadius = 200.f;
	
	UPROPERTY(EditAnywhere, Category = "Properties | Damage")
	float DamageOuterRadius = 500.f;
	
	UPROPERTY(EditAnywhere, Category = "Properties | Effects")
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere, Category = "Properties | Effects")
	UParticleSystem* Tracer;
	
	UPROPERTY(EditAnywhere, Category = "Properties | Effects")
	class USoundCue* ImpactSound;
	
	UPROPERTY(EditAnywhere, Category = "Properties | Effects")
	class UNiagaraSystem* TrailSystem;
	
	UPROPERTY(EditAnywhere, Category = "Properties | Settings")
	float DestroyTime = 3.f;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UBoxComponent* CollisionBox;
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UParticleSystemComponent* TracerComponent;

public:
	
	bool bUseServerSideRewind = false;
	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity;

protected:
	
	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;
	
	FTimerHandle DestroyTimer;

public:
	
	ASProjectile();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void Tick(float DeltaTime) override;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;

	virtual void ApplyDamage(const UObject* WorldContextObject, const FVector& Location, AActor* DamagedActor, float BaseDamage, IAbilitySystemInterface* Source, AActor* DamageCauser, TSubclassOf<class UGameplayEffect> DamageEffectClass, ECollisionChannel DamageChannel = ECC_Visibility) const;

protected:
	
	virtual void BeginPlay() override;

	void StartDestroyTimer();
	
	void DestroyTimerFinished();
	
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	void SpawnTrailSystem();

	void ExplodeDamage(const UObject* WorldContextObject, TSubclassOf<UGameplayEffect> DamageEffect, const FVector& Location, IAbilitySystemInterface* Source, AActor* DamageCauser, float BaseDamage, ECollisionChannel DamageChannel) const;
};


