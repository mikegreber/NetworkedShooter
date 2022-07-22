// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

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

	UPROPERTY(EditAnywhere, Category = "Pickup | Gives")
	TArray<TSubclassOf<class UGameplayAbility>> PickupAbilities;

	UPROPERTY(EditAnywhere, Category = "Pickup | Gives")
	TArray<FGameplayEffectInfo> PickupEffects;

	UPROPERTY(EditAnywhere, Category= "Pickup | Gives")
	TArray<struct FGameplayTagStack> PickupTags;

	UPROPERTY(EditAnywhere, Category = "Pickup | Effects")
	class USoundCue* PickupSound;

	UPROPERTY(EditAnywhere, Category = "Pickup | Effects")
	ECustomDepthColor OutlineColor = ECustomDepthColor::CDC_Purple;

	UPROPERTY(VisibleAnywhere, Category = "Pickup | Effects")
	class UNiagaraComponent* PickupEffectComponent;

	UPROPERTY(EditAnywhere, Category = "Pickup | Effects")
	class UNiagaraSystem* PickupEffect;

	UPROPERTY(EditAnywhere, Category = "Pickup | Effects")
	float BaseTurnRate = 45.f;
	
	UPROPERTY(VisibleAnywhere)
	class USphereComponent* OverlapSphere;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* PickupMesh;
	
	FTimerHandle BindOverlapTimer;
	
	float BindOverlapTime = 0.25f;
	
public:	
	ASPickup();

	virtual void Tick(float DeltaTime) override;
	
	virtual void Destroyed() override;
	
protected:
	
	virtual void BeginPlay() override;

	void BindOverlap();
	
	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	
	bool GiveTags(AActor* OtherActor);
	
	bool GiveEffects(AActor* OtherActor);
	
	bool GiveAbilities(AActor* OtherActor);
	
};
