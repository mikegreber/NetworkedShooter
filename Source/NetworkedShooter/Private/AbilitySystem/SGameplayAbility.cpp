// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SGameplayAbility.h"

#include "AbilitySystemComponent.h"

void USGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (bActivateAbilityOnGranted)
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);

		if (bRemoveAbilityOnEnd)
		{
			ActorInfo->AbilitySystemComponent->SetRemoveAbilityOnEnd(Spec.Handle);
		}
	}
}
