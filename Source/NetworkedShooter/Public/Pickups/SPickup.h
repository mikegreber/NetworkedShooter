// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/CustomDepth.h"
#include "SPickup.generated.h"

UCLASS()
class NETWORKEDSHOOTER_API ASPickup : public AActor
{
	GENERATED_BODY()
	
public:	
	ASPickup();

	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;
	
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	
	void SetCustomDepthColor(ECustomDepthColor Color) const;

	UPROPERTY(EditAnywhere, Category = "Pickup | Effects")
	float BaseTurnRate = 45.f;

	UPROPERTY(EditAnywhere, Category = "Pickup | Effects")
	class UNiagaraSystem* PickupEffect;
	
private:

	UPROPERTY(EditAnywhere)
	class USphereComponent* OverlapSphere;

	UPROPERTY(EditAnywhere)
	class UStaticMeshComponent* PickupMesh;

	UPROPERTY(EditAnywhere, Category = "Pickup | Effects")
	class USoundCue* PickupSound;

	UPROPERTY(EditAnywhere, Category = "Pickup | Effects")
	ECustomDepthColor OutlineColor = ECustomDepthColor::CDC_Purple;

	UPROPERTY(VisibleAnywhere, Category = "Pickup | Effects")
	class UNiagaraComponent* PickupEffectComponent;

	FTimerHandle BindOverlapTimer;
	float BindOverlapTime = 0.25f;
	void BindOverlapTimerFinished();
public:	

};
