// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayTagStack.h"
#include "SAttributeSet.generated.h"

// setter with broadcast
#define GAMEPLAYATTRIBUTE_VALUE_SETTER_BROADCAST(PropertyName) \
	FOnAttributeChanged On##PropertyName##Changed; \
	FORCEINLINE void Set##PropertyName(float NewVal) \
	{ \
		UAbilitySystemComponent* AbilityComp = GetOwningAbilitySystemComponent(); \
		if (ensure(AbilityComp)) \
		{ \
			const float OldVal = AbilityComp->GetNumericAttributeBase(Get##PropertyName##Attribute());\
			AbilityComp->SetNumericAttributeBase(Get##PropertyName##Attribute(), NewVal); \
			On##PropertyName##Changed.Broadcast(AbilityComp->GetNumericAttributeBase(Get##PropertyName##Attribute()), OldVal); \
		}; \
	}

// Uses macros from AttributeSet.h
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

#define ATTRIBUTE_ACCESSORS_BROADCAST(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER_BROADCAST(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

#define GAMEPLAYATTRIBUTE_REPNOTIFY_BROADCAST(ClassName, PropertyName, OldValue) \
	GAMEPLAYATTRIBUTE_REPNOTIFY(ClassName, PropertyName, OldValue); \
	On##PropertyName##Changed.Broadcast(Get##PropertyName##(), OldValue.GetCurrentValue());

// need to substitute macro call, can't build with this
#define ON_REP_BUILDER(PropertyName) \
	UFUNCTION() \
	void OnRep_##PropertyName##(const FGameplayAttributeData& Old##PropertyName##);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChanged, float, NewValue, float, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKilled, AController*, Instigator);


USTRUCT(BlueprintType)
struct FMyStruct
{
	GENERATED_BODY()

	FMyStruct() : Test(0) {}
	
	UPROPERTY()
	int32 Test;
};

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API USAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData Damage;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health)
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Shield)
	FGameplayAttributeData Shield;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxShield)
	FGameplayAttributeData MaxShield;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Speed)
	FGameplayAttributeData Speed;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_SprintSpeed)
	FGameplayAttributeData SprintSpeed;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ADSSpeed)
	FGameplayAttributeData ADSSpeed;
	
	FOnKilled OnKilled;

	UPROPERTY() AActor* Owner;
	
public:
	USAttributeSet();

	void SetOwner(AActor* NewOwner) { Owner = NewOwner; }

	// AttributeSet Overrides
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	ATTRIBUTE_ACCESSORS(USAttributeSet, Damage);
	ATTRIBUTE_ACCESSORS(USAttributeSet, Speed);
	ATTRIBUTE_ACCESSORS(USAttributeSet, SprintSpeed);
	ATTRIBUTE_ACCESSORS(USAttributeSet, ADSSpeed);
	ATTRIBUTE_ACCESSORS_BROADCAST(USAttributeSet, Health);
	ATTRIBUTE_ACCESSORS_BROADCAST(USAttributeSet, MaxHealth);
	ATTRIBUTE_ACCESSORS_BROADCAST(USAttributeSet, Shield);
	ATTRIBUTE_ACCESSORS_BROADCAST(USAttributeSet, MaxShield);

protected:

	void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);
	
	UFUNCTION() virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
	UFUNCTION() virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
	UFUNCTION() virtual void OnRep_Shield(const FGameplayAttributeData& OldShield);
	UFUNCTION() virtual void OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield);
	UFUNCTION() virtual void OnRep_Speed(const FGameplayAttributeData& OldSpeed);
	UFUNCTION() virtual void OnRep_SprintSpeed(const FGameplayAttributeData& OldSpeed);
	UFUNCTION() virtual void OnRep_ADSSpeed(const FGameplayAttributeData& OldSpeed);

};
