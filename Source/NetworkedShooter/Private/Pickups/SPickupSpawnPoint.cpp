
#include "Pickups/SPickupSpawnPoint.h"

#include "Pickups/SPickup.h"

ASPickupSpawnPoint::ASPickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void ASPickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority()) StartSpawnPickupTimer();
}

void ASPickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	const float SpawnTime = FMath::FRandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(
		TimerHandle,
		this,
		&ASPickupSpawnPoint::SpawnPickup,
		SpawnTime
	);
}

void ASPickupSpawnPoint::SpawnPickup()
{
	if (!PickupClasses.IsEmpty())
	{
		const int32 Selection = FMath::RandRange(0, PickupClasses.Num() - 1);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedPickup = GetWorld()->SpawnActor<ASPickup>(
			PickupClasses[Selection].LoadSynchronous(),
			GetActorTransform(),
			SpawnParams
		);
		
		SpawnedPickup->OnDestroyed.AddDynamic(this, &ASPickupSpawnPoint::StartSpawnPickupTimer);
	}
}




