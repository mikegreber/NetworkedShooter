// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "SBulletCasing.generated.h"

UCLASS()
class NETWORKEDSHOOTER_API ASBulletCasing : public AActor
{
	GENERATED_BODY()
	
public:	

	ASBulletCasing();

	virtual void PostInitializeComponents() override;
protected:
	
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:

	UPROPERTY(VisibleAnywhere, Category = Components)
	UStaticMeshComponent* CasingMesh;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess="true"))
	float ShellEjectionImpulse;

	UPROPERTY(EditAnywhere, Category = "Effects")
	class USoundCue* ShellSound;

	bool bHasHit;
};
