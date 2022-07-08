// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SPickupSpawnPoint.generated.h"

UCLASS()
class NETWORKEDSHOOTER_API ASPickupSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	ASPickupSpawnPoint();
	
	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;

	void SpawnPickup();

	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor = nullptr);
	
	void SpawnPickupTimerFinished();

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TArray<TSubclassOf<class ASPickup>> PickupClasses;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TArray<TSoftClassPtr<class ASPickup>> SoftPickupClasses;
	
	UPROPERTY()
	ASPickup* SpawnedPickup;
private:	

	FTimerHandle SpawnPickupTimer;
	
	UPROPERTY(EditAnywhere, Category = "Spawning")
	float SpawnPickupTimeMin;
	
	UPROPERTY(EditAnywhere, Category = "Spawning")
	float SpawnPickupTimeMax;
};
