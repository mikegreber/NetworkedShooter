// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "SAttributeSet.generated.h"

// setter with broadcast
#define GAMEPLAYATTRIBUTE_VALUE_SETTER_BROADCAST(PropertyName, Delegate) \
	FORCEINLINE void Set##PropertyName(float NewVal) \
	{ \
		UAbilitySystemComponent* AbilityComp = GetOwningAbilitySystemComponent(); \
		if (ensure(AbilityComp)) \
		{ \
			const float OldVal = AbilityComp->GetNumericAttributeBase(Get##PropertyName##Attribute());\
			AbilityComp->SetNumericAttributeBase(Get##PropertyName##Attribute(), NewVal); \
			Delegate.Broadcast(AbilityComp->GetNumericAttributeBase(Get##PropertyName##Attribute()), OldVal); \
		}; \
	}

// Uses macros from AttributeSet.h
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

#define ATTRIBUTE_ACCESSORS_BROADCAST(ClassName, PropertyName, Delegate) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER_BROADCAST(PropertyName, Delegate) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChanged, float, NewValue, float, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKilled, AController*, Instigator);

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API USAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health)
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Shield)
	FGameplayAttributeData Shield;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxShield)
	FGameplayAttributeData MaxShield;

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData Damage;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Speed)
	FGameplayAttributeData Speed;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_SprintSpeed)
	FGameplayAttributeData SprintSpeed;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ADSSpeed)
	FGameplayAttributeData ADSSpeed;
	
	FOnAttributeChanged OnHealthChanged;
	FOnAttributeChanged OnMaxHealthChanged;
	FOnAttributeChanged OnShieldChanged;
	FOnAttributeChanged OnMaxShieldChanged;
	FOnAttributeChanged OnSpeedChanged;
	FOnKilled OnKilled;

	UPROPERTY() AActor* Owner;
	

	USAttributeSet();

	void SetOwner(AActor* NewOwner) { Owner = NewOwner; }

	// AttributeSet Overrides
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	ATTRIBUTE_ACCESSORS(USAttributeSet, Damage);
	ATTRIBUTE_ACCESSORS(USAttributeSet, SprintSpeed);
	ATTRIBUTE_ACCESSORS(USAttributeSet, ADSSpeed);
	ATTRIBUTE_ACCESSORS_BROADCAST(USAttributeSet, Health, OnHealthChanged);
	ATTRIBUTE_ACCESSORS_BROADCAST(USAttributeSet, MaxHealth, OnMaxHealthChanged);
	ATTRIBUTE_ACCESSORS_BROADCAST(USAttributeSet, Shield, OnShieldChanged);
	ATTRIBUTE_ACCESSORS_BROADCAST(USAttributeSet, MaxShield, OnMaxShieldChanged);
	ATTRIBUTE_ACCESSORS_BROADCAST(USAttributeSet, Speed, OnSpeedChanged);

protected:

	void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);
	
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	virtual void OnRep_Shield(const FGameplayAttributeData& OldShield);

	UFUNCTION()
	virtual void OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield);

	UFUNCTION()
	virtual void OnRep_Speed(const FGameplayAttributeData& OldSpeed);

	UFUNCTION()
	virtual void OnRep_SprintSpeed(const FGameplayAttributeData& OldSpeed);

	UFUNCTION()
	virtual void OnRep_ADSSpeed(const FGameplayAttributeData& OldSpeed);
};
