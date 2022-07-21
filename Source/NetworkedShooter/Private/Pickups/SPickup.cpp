// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/SPickup.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ASPickup::ASPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");
	
	OverlapSphere = CreateDefaultSubobject<USphereComponent>("OverlapSphere");
	OverlapSphere->SetupAttachment(RootComponent);
	OverlapSphere->SetSphereRadius(150.f);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	OverlapSphere->AddLocalOffset(FVector(0.f,0.f,85.f));

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>("PickupMesh");
	PickupMesh->SetupAttachment(OverlapSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetRelativeScale3D(FVector(5.f,5.f,5.f));
	PickupMesh->SetRenderCustomDepth(true);
	SetCustomDepthColor(OutlineColor);

	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>("PickupEffectComponent");
	PickupEffectComponent->SetupAttachment(RootComponent);
}

void ASPickup::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(
			BindOverlapTimer,
			this,
			&ASPickup::BindOverlapTimerFinished,
			BindOverlapTime
		);
	}
}

void ASPickup::BindOverlapTimerFinished()
{
	OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &ASPickup::OnSphereBeginOverlap);

	// check if already have overlaps
	TArray<AActor*> OverlappingActors;
	OverlapSphere->GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors)
	{
		
		OverlapSphere->OnComponentBeginOverlap.Broadcast(nullptr, Actor, nullptr, 0, false, FHitResult());
	}
}

void ASPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (const IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(OtherActor))
	{
		UAbilitySystemComponent* ASC = Interface->GetAbilitySystemComponent();
		for (const TSubclassOf<UGameplayAbility> Ability : PickupAbilities)
		{
			ASC->GiveAbility(Ability);
		}

		for (const auto& [GameplayEffect, Level] : PickupEffects)
		{
			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GameplayEffect, Level, ASC->MakeEffectContext());
			if (FGameplayEffectSpec* Spec = SpecHandle.Data.Get())
			{
				static FGameplayTag DataTag_Overtime = FGameplayTag::RequestGameplayTag(FName("DataTag.OverTime"));
				
				FGameplayTagContainer TagContainer;
				Spec->GetAllAssetTags(TagContainer);
				
				if (TagContainer.HasTag(DataTag_Overtime))
				{
					Spec->Period = 0.05f;
					Spec->SetSetByCallerMagnitude(DataTag_Overtime, Level * (Spec->GetPeriod() / Spec->GetDuration()));
				}
			}
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
		Destroy();
	}
}

void ASPickup::SetCustomDepthColor(ECustomDepthColor Color) const
{
	if (PickupMesh)
	{
		PickupMesh->SetCustomDepthStencilValue(static_cast<int32>(Color));
	}
}

void ASPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (PickupMesh)
	{
		PickupMesh->AddWorldRotation(FRotator(0.f, BaseTurnRate * DeltaTime, 0.f));
	}
}

void ASPickup::Destroyed()
{
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			PickupSound,
			GetActorLocation()
		);
	}

	if (PickupEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			PickupEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}
	
	Super::Destroyed();
}

