// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/SPickup_GameplayEffect.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "NetworkedShooter/NetworkedShooter.h"

void ASPickup_GameplayEffect::BeginPlay()
{
	Super::BeginPlay();

	// if (!HasAuthority())
	// {
	// 	GetWorldTimerManager().SetTimer(
	// 		BindOverlapTimer,
	// 		this,
	// 		&ASPickup_GameplayEffect::BindOverlapTimerFinished,
	// 		BindOverlapTime
	// 	);
	// }
}

void ASPickup_GameplayEffect::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (const IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(OtherActor))
	{
		UAbilitySystemComponent* ASC = Interface->GetAbilitySystemComponent();
		
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		if (const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectToAdd, 1, EffectContext); SpecHandle.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("%s %s Applying Gameplay Effect"), __FUNCTIONW__, *NET_ROLE_STRING_ACTOR)
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			Destroy();
		}
	}
}
