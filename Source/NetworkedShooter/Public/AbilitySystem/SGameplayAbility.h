// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API USGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Ability")
	bool bActivateAbilityOnGranted;

	UPROPERTY(EditAnywhere, Category = "Ability")
	bool bRemoveAbilityOnEnd;
	
public:
	
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
};
