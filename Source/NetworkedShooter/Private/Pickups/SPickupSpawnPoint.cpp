
#include "Pickups/SPickupSpawnPoint.h"

#include "Pickups/SPickup.h"

ASPickupSpawnPoint::ASPickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void ASPickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		StartSpawnPickupTimer();
	}
}

void ASPickupSpawnPoint::SpawnPickup()
{
	if (HasAuthority() && !SoftPickupClasses.IsEmpty())
	{
		const int32 Selection = FMath::RandRange(0, SoftPickupClasses.Num() - 1);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedPickup = GetWorld()->SpawnActor<ASPickup>(
			SoftPickupClasses[Selection].LoadSynchronous(),
			GetActorTransform(),
			SpawnParams
		);
		SpawnedPickup->OnDestroyed.AddDynamic(this, &ASPickupSpawnPoint::StartSpawnPickupTimer);
	}
}

void ASPickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	const float SpawnTime = FMath::FRandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);
	GetWorldTimerManager().SetTimer(
		SpawnPickupTimer,
		this,
		&ASPickupSpawnPoint::SpawnPickupTimerFinished,
		SpawnTime
	);
}

void ASPickupSpawnPoint::SpawnPickupTimerFinished()
{
	if (HasAuthority())
	{
		SpawnPickup();
	}
}

void ASPickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

