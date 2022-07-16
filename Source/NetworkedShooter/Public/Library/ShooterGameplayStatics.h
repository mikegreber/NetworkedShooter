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
	/** Hurt locally authoritative actors within the radius. Will only hit components that block the Visibility channel.
 * @param BaseDamage - The base damage to apply, i.e. the damage at the origin.
 * @param Origin - Epicenter of the damage area.
 * @param DamageInnerRadius - Radius of the full damage area, from Origin
 * @param DamageOuterRadius - Radius of the minimum damage area, from Origin
 * @param DamageFalloff - Falloff exponent of damage from DamageInnerRadius to DamageOuterRadius
 * @param DamageTypeClass - Class that describes the damage that was done.
 * @param IgnoreActors - List of Actors to ignore
 * @param DamageChannel - Damage will only be applied to victim trace is blocked on this channel
 * @param DamageCauser - Actor that actually caused the damage (e.g. the grenade that exploded)
 * @param InstigatedByController - Controller that was responsible for causing this damage (e.g. player who threw the grenade)
 * @param DamagePreventionChannel - Damage will not be applied to victim if there is something between the origin and the victim which blocks traces on this channel
 * @return true if damage was applied to at least one actor.
 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Game|Damage", meta=(WorldContext="WorldContextObject", AutoCreateRefTerm="IgnoreActors"))
	static bool ApplyRadialDamageWithFalloff(const UObject* WorldContextObject, float BaseDamage, float MinimumDamage, const FVector& Origin, float DamageInnerRadius, float DamageOuterRadius, float DamageFalloff, TSubclassOf<class UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, ECollisionChannel DamageChannel = ECC_Visibility, AActor* DamageCauser = NULL, AController* InstigatedByController = NULL, ECollisionChannel DamagePreventionChannel = ECC_Visibility);

	static float ApplyDamage(AActor* DamagedActor, float BaseDamage, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass);

	// same as UGameplayStatics, but always uses GravityOverrideZ
	static bool PredictProjectilePath(const UObject* WorldContextObject, const FPredictProjectilePathParams& PredictParams, FPredictProjectilePathResult& PredictResult);

	template<class T>
	static void GetActorsInRadius(const UObject* WorldContextObject, const FVector& Origin, float Radius, TArray<T*>& OutActors, const TArray<AActor*>& IgnoreActors = TArray<AActor*>());
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