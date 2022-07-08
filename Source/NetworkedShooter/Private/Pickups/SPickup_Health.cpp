// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/SPickup_Health.h"

#include "Character/SCharacter.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SBuffComponent.h"

ASPickup_Health::ASPickup_Health()
{
}

void ASPickup_Health::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (const ASCharacter* Character = Cast<ASCharacter>(OtherActor))
	{
		if (USBuffComponent* BuffComponent = Character->GetBuffComponent())
		{
			BuffComponent->Heal(HealAmount, HealingTime);
			Destroy();
		}
	}
}
