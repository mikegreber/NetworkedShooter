// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "SAttributeSet.h"
#include "SAbilitySystemComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FReceivedDamageDelegate, class USAbilitySystemComponent*, SourceASC, float, UnmitigatedDamage, float, MitigatedDamage);


UCLASS()
class NETWORKEDSHOOTER_API USAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

	UPROPERTY()
	const USAttributeSet* AttributeSet;

	

public:
	virtual void BeginPlay() override;
};
