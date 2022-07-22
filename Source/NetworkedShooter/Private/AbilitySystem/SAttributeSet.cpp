// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Character/SCharacter.h"
#include "Library/ShooterGameplayStatics.h"

#include "Net/UnrealNetwork.h"
#include "NetworkedShooter/NetworkedShooter.h"



USAttributeSet::USAttributeSet()
{}

void USAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(USAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USAttributeSet, Speed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USAttributeSet, SprintSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USAttributeSet, ADSSpeed, COND_None, REPNOTIFY_Always);
}

void USAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetMaxHealthAttribute())
	{
		AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute());
	}
	else if (Attribute == GetMaxShieldAttribute())
	{
		AdjustAttributeForMaxChange(Shield, MaxShield, NewValue, GetShieldAttribute());
	}
	else if (Attribute == GetSpeedAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 150.f, 2000.f);
	}
}

void USAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData & AffectedAttribute, const FGameplayAttributeData & MaxAttribute, float NewMaxValue, const FGameplayAttribute & AffectedAttributeProperty)
{
	UAbilitySystemComponent* AbilityComp = GetOwningAbilitySystemComponent();
	
	const float CurrentMaxValue = MaxAttribute.GetCurrentValue();
	if (!FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue) && AbilityComp)
	{
		// Change current value to maintain the current Val / Max percent
		const float CurrentValue = AffectedAttribute.GetCurrentValue();
		float NewDelta = (CurrentMaxValue > 0.f) ? (CurrentValue * NewMaxValue / CurrentMaxValue) - CurrentValue : NewMaxValue;

		AbilityComp->ApplyModToAttributeUnsafe(AffectedAttributeProperty, EGameplayModOp::Additive, NewDelta);
	}
}

void ReadAbilityActorInfo(AActor*& OutActor, AController*& OutController, TSharedPtr<FGameplayAbilityActorInfo> ActorInfo)
{
	OutActor = nullptr;
	OutController = nullptr;
	if (ActorInfo.IsValid() && ActorInfo->AvatarActor.IsValid())
	{
		OutActor = ActorInfo->AvatarActor.Get();
		OutController = ActorInfo->PlayerController.Get();
	}
}

void USAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	UAbilitySystemComponent* SourceComponent = Context.GetOriginalInstigatorAbilitySystemComponent();
	const FGameplayTagContainer& SourceTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
	FGameplayTagContainer SpecAssetTags;
	Data.EffectSpec.GetAllAssetTags(SpecAssetTags);
	
	// get the Target actor, should be our owner
	AActor* TargetActor = nullptr;
	AController* TargetController = nullptr;
	ReadAbilityActorInfo(TargetActor, TargetController, Data.Target.AbilityActorInfo);
	
	// get the Source actor
	AActor* SourceActor = nullptr;
	AController* SourceController = nullptr;
	if (SourceComponent) ReadAbilityActorInfo(SourceActor, SourceController, SourceComponent->AbilityActorInfo);

	FGameplayAttribute& Attribute = Data.EvaluatedData.Attribute;
	
	if (Attribute == GetDamageAttribute())
	{
		FHitResult HitResult;
		if (Context.GetHitResult()) HitResult = *Context.GetHitResult();
		
		if (GetDamage() > 0.f)
		{
			float LocalDamageDone = GetDamage();
			SetDamage(0.f);
			
			if (GetHealth() > 0.f)
			{
				if (GetShield() > 0.f)
				{
                    SetShield(UShooterGameplayStatics::ClampWithOverflow<float>(GetShield() - LocalDamageDone, 0.f, GetMaxShield(), LocalDamageDone));
				}
				
				SetHealth(FMath::Clamp(GetHealth() - LocalDamageDone, 0.f, GetMaxHealth()));

				if (GetHealth() == 0)
				{
					OnKilled.Broadcast(SourceController);
				}
			}
		}
	}
	else if (Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Attribute == GetShieldAttribute())
	{
		SetShield(FMath::Clamp(GetShield(), 0.0f, GetMaxShield()));
	}
}

void USAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY_BROADCAST(USAttributeSet, Health, OldHealth);
}

void USAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY_BROADCAST(USAttributeSet, MaxHealth, OldMaxHealth);
}

void USAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY_BROADCAST(USAttributeSet, Shield, OldShield);
}

void USAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY_BROADCAST(USAttributeSet, MaxShield, OldMaxShield);
}

void USAttributeSet::OnRep_Speed(const FGameplayAttributeData& OldSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USAttributeSet, Speed, OldSpeed);
}

void USAttributeSet::OnRep_SprintSpeed(const FGameplayAttributeData& OldSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USAttributeSet, SprintSpeed, OldSpeed);
}

void USAttributeSet::OnRep_ADSSpeed(const FGameplayAttributeData& OldSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USAttributeSet, ADSSpeed, OldSpeed);
}



