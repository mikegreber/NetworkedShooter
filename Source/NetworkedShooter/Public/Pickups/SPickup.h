// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Types/CustomDepth.h"
#include "SPickup.generated.h"

USTRUCT(BlueprintType)
struct FGameplayEffectInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UGameplayEffect> GameplayEffect;

	UPROPERTY(EditAnywhere)
	int32 Level;
};

UCLASS()
class NETWORKEDSHOOTER_API ASPickup : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Pickup | Pickup")
	TArray<TSubclassOf<class UGameplayAbility>> PickupAbilities;

	UPROPERTY(EditAnywhere, Category = "Pickup | Pickup")
	TArray<FGameplayEffectInfo> PickupEffects;

	UPROPERTY(EditAnywhere, Category = "Pickup | Effects")
	class USoundCue* PickupSound;

	UPROPERTY(EditAnywhere, Category = "Pickup | Effects")
	ECustomDepthColor OutlineColor = ECustomDepthColor::CDC_Purple;

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* OverlapSphere;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* PickupMesh;

	UPROPERTY(VisibleAnywhere, Category = "Pickup | Effects")
	class UNiagaraComponent* PickupEffectComponent;
	
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

	FTimerHandle BindOverlapTimer;
	float BindOverlapTime = 0.25f;
	void BindOverlapTimerFinished();
	
private:
	

	
};
