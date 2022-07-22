// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "ShooterGameplayStatics.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKEDSHOOTER_API UShooterGameplayStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	static void ApplyGameplayEffect(const class IAbilitySystemInterface* Source, const IAbilitySystemInterface* Target, TSubclassOf<class UGameplayEffect> EffectClass, float Level);

	static bool ApplyGameplayEffectWithRadialFalloff(const UObject* WorldContextObject, TSubclassOf<UGameplayEffect> EffectClass, const FVector& Origin, float BaseLevel, float MinimumLevel, float InnerRadius, float OuterRadius, float Falloff, AActor* EffectCauser = nullptr, IAbilitySystemInterface* Source = nullptr, const TArray<AActor*>& IgnoreActors = TArray<AActor*>(), ECollisionChannel DamageChannel = ECC_Visibility, ECollisionChannel DamagePreventionChannel = ECC_Visibility);

	// same as UGameplayStatics, but always uses GravityOverrideZ
	static bool PredictProjectilePath(const UObject* WorldContextObject, const FPredictProjectilePathParams& PredictParams, FPredictProjectilePathResult& PredictResult);

	static void PlayMontage(UAnimInstance* Instance, UAnimMontage* Montage, FName SectionName = NAME_None);

	template<class T>
	static void GetActorsInRadius(const UObject* WorldContextObject, const FVector& Origin, float Radius, TArray<T*>& OutActors, const TArray<AActor*>& IgnoreActors = TArray<AActor*>());

	template<typename T>
	static T ClampWithOverflow(T Value, T Min, T Max, T& OutOverflow);
};

template <class T>
void UShooterGameplayStatics::GetActorsInRadius(const UObject* WorldContextObject, const FVector& Origin, float Radius, TArray<T*>& OutActors, const TArray<AActor*>& IgnoreActors)
{
	FCollisionQueryParams SphereParams;
	SphereParams.bTraceComplex = false;
	SphereParams.AddIgnoredActors(IgnoreActors);
	
	// query scene to see what we hit
	TArray<FOverlapResult> Overlaps;
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects), FCollisionShape::MakeSphere(Radius), SphereParams);
	}
	
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (AActor* Actor = Overlap.OverlapObjectHandle.FetchActor(); !OutActors.Contains(Actor))
		{
			if (T* Character = Cast<T>(Actor))
			{
				OutActors.Add(Character);
			}
		}
	}
}

template<typename T>
T UShooterGameplayStatics::ClampWithOverflow(T Value, T Min, T Max, T& OutOverflow)
{
	if (Value < Min)
	{
		OutOverflow = Min - Value;
		return Min;
	}
	if (Value > Max)
	{
		OutOverflow = Value - Max;
		return Max;
	}
	OutOverflow = T();
	return Value;
}