// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SProjectile.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/BoxComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Library/ShooterGameplayStatics.h"
#include "Sound/SoundCue.h"

ASProjectile::ASProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetCollisionProfileName("Projectile");
	
	SetRootComponent(CollisionBox);
}

#if WITH_EDITOR
void ASProjectile::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property->GetFName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ASProjectile, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ASProjectile, ProjectileGravityScale))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->ProjectileGravityScale = ProjectileGravityScale;
		}
	}
}
#endif

void ASProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ASProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(DestroyTimer, this, &ASProjectile::DestroyTimerFinished, DestroyTime);
}

void ASProjectile::DestroyTimerFinished()
{
	Destroy();
}

void ASProjectile::Destroyed()
{
	Super::Destroyed();
	
	if (ImpactParticles)
    {
    	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
    }
    
    if (ImpactSound)
    {
    	UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, GetActorLocation());
    }
}

void ASProjectile::ApplyDamage(const UObject* WorldContextObject, const FVector& Location, AActor* DamagedActor, float BaseDamage, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass, ECollisionChannel DamageChannel) const
{
	ExplodeDamage(WorldContextObject, Location, EventInstigator, DamageCauser, BaseDamage, DamageChannel);
}


void ASProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			RootComponent,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}

	CollisionBox->OnComponentHit.AddDynamic(this, &ASProjectile::OnHit);
}


void ASProjectile::SpawnTrailSystem()
{
	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
}

void ASProjectile::ExplodeDamage(const UObject* WorldContextObject, const FVector& Location, AController* EventInstigator, AActor* DamageCauser, float BaseDamage, ECollisionChannel DamageChannel) const
{
	if (HasAuthority())
	{
		UShooterGameplayStatics::ApplyRadialDamageWithFalloff(
			WorldContextObject,
			BaseDamage,
			10.f,
			Location,
			DamageInnerRadius,
			DamageOuterRadius,
			1.f,
			UDamageType::StaticClass(),
			TArray<AActor*>(),
			DamageChannel,
			DamageCauser,
			EventInstigator
		);
	}
}

void ASProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Destroy();
}


void ASProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

