// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SPickupSpawnPoint.generated.h"

UCLASS()
class NETWORKEDSHOOTER_API ASPickupSpawnPoint : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TArray<TSoftClassPtr<class ASPickup>> PickupClasses;
	
	UPROPERTY(EditAnywhere, Category = "Spawning")
	float SpawnPickupTimeMin;
	
	UPROPERTY(EditAnywhere, Category = "Spawning")
	float SpawnPickupTimeMax;
	
	UPROPERTY() ASPickup* SpawnedPickup;
	
public:
	
	ASPickupSpawnPoint();

protected:

	virtual void BeginPlay() override;

	void SpawnPickup();

	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor = nullptr);
	
};
