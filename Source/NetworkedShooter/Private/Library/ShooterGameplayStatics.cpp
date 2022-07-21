// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/ShooterGameplayStatics.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Kismet/GameplayStatics.h"

/** @RETURN True if weapon trace from Origin hits component VictimComp.  OutHitResult will contain properties of the hit. */
static bool ComponentIsDamageableFrom(UPrimitiveComponent* VictimComp, FVector const& Origin, AActor const* IgnoredActor, const TArray<AActor*>& IgnoreActors, ECollisionChannel TraceChannel, FHitResult& OutHitResult)
{
	FCollisionQueryParams LineParams(SCENE_QUERY_STAT(ComponentIsVisibleFrom), true, IgnoredActor);
	LineParams.AddIgnoredActors( IgnoreActors );

	// Do a trace from origin to middle of box
	UWorld* const World = VictimComp->GetWorld();
	check(World);

	FVector const TraceEnd = VictimComp->Bounds.Origin;
	FVector TraceStart = Origin;
	if (Origin == TraceEnd)
	{
		// tiny nudge so LineTraceSingle doesn't early out with no hits
		TraceStart.Z += 0.01f;
	}

	// Only do a line trace if there is a valid channel, if it is invalid then result will have no fall off
	if (TraceChannel != ECollisionChannel::ECC_MAX)
	{
		bool const bHadBlockingHit = World->LineTraceSingleByChannel(OutHitResult, TraceStart, TraceEnd, TraceChannel, LineParams);
		//::DrawDebugLine(World, TraceStart, TraceEnd, FLinearColor::Red, true);

		// If there was a blocking hit, it will be the last one
		if (bHadBlockingHit)
		{
			if (OutHitResult.Component == VictimComp)
			{
				// if blocking hit was the victim component, it is visible
				return true;
			}
			else
			{
				// if we hit something else blocking, it's not
				UE_LOG(LogDamage, Log, TEXT("Radial Damage to %s blocked by %s (%s)"), *GetNameSafe(VictimComp), *OutHitResult.GetHitObjectHandle().GetName(), *GetNameSafe(OutHitResult.Component.Get()));
				return false;
			}
		}
	}
	else
	{
		UE_LOG(LogDamage, Warning, TEXT("ECollisionChannel::ECC_MAX is not valid! No falloff is added to damage"));
	}

	// didn't hit anything, assume nothing blocking the damage and victim is consequently visible
	// but since we don't have a hit result to pass back, construct a simple one, modeling the damage as having hit a point at the component's center.
	FVector const FakeHitLoc = VictimComp->GetComponentLocation();
	FVector const FakeHitNorm = (Origin - FakeHitLoc).GetSafeNormal();		// normal points back toward the epicenter
	OutHitResult = FHitResult(VictimComp->GetOwner(), VictimComp, FakeHitLoc, FakeHitNorm);
	return true;
}

static float GetDamageScale(float DistanceFromEpicenter, float InnerRadius, float OuterRadius, float DamageFalloff)
{
	float const ValidatedInnerRadius = FMath::Max(0.f, InnerRadius);
	float const ValidatedOuterRadius = FMath::Max(OuterRadius, ValidatedInnerRadius);
	float const ValidatedDist = FMath::Max(0.f, DistanceFromEpicenter);

	if (ValidatedDist >= ValidatedOuterRadius)
	{
		// outside the radius, no effect
		return 0.f;
	}

	if ( (DamageFalloff == 0.f)	|| (ValidatedDist <= ValidatedInnerRadius) )
	{
		// no falloff or inside inner radius means full effect
		return 1.f;
	}

	// calculate the interpolated scale
	float DamageScale = 1.f - ( (ValidatedDist - ValidatedInnerRadius) / (ValidatedOuterRadius - ValidatedInnerRadius) );
	DamageScale = FMath::Pow(DamageScale, DamageFalloff);

	return DamageScale;
}

static float CalculateRadialDamage(float Damage, const FVector& Origin, TArray<struct FHitResult>& ComponentHits, float InnerRadius, float OuterRadius, float DamageFalloff, float MinimumDamage)
{
	float ActualDamage = Damage;
	
	float ClosestHitDistSq = MAX_FLT;
	for (const FHitResult& Hit : ComponentHits)
	{
		float const DistSq = (Hit.ImpactPoint - Origin).SizeSquared();
		if (DistSq < ClosestHitDistSq)
		{
			ClosestHitDistSq = DistSq;
		}
	}

	float const RadialDamageScale = GetDamageScale(FMath::Sqrt(ClosestHitDistSq), InnerRadius, OuterRadius, DamageFalloff);

	ActualDamage = FMath::Lerp(MinimumDamage, ActualDamage, FMath::Max(0.f, RadialDamageScale));

	return ActualDamage;
}

void UShooterGameplayStatics::ApplyGameplayEffect(const IAbilitySystemInterface* Source, const IAbilitySystemInterface* Target, TSubclassOf<UGameplayEffect> EffectClass, float Level)
{
	if (!EffectClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s called with null EffectClass"), __FUNCTIONW__)
		return;
	}
	
	if (Source)
	{
		UAbilitySystemComponent* OwnerASC = Source->GetAbilitySystemComponent();
		FGameplayEffectContextHandle ContextHandle = OwnerASC->MakeEffectContext();
	
		FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(EffectClass, Level, ContextHandle);
	
		if (SpecHandle.Data.IsValid())
		{
			OwnerASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), Target->GetAbilitySystemComponent());
		}
	}
	else if (Target)
	{
		UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
		FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
	
		FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectClass, Level, ContextHandle);
	
		if (SpecHandle.Data.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s Source and Target are null, effect not applied"), __FUNCTIONW__)
	}
	
}

bool UShooterGameplayStatics::ApplyRadialGameplayEffectWithFalloff(const UObject* WorldContextObject, TSubclassOf<UGameplayEffect> EffectClass, const FVector& Origin, float BaseLevel, float MinimumLevel, float InnerRadius, float OuterRadius, float Falloff, AActor* EffectCauser, IAbilitySystemInterface* Source, const TArray<AActor*>& IgnoreActors, ECollisionChannel DamageChannel, ECollisionChannel DamagePreventionChannel)
{
	FCollisionQueryParams SphereParams;
	SphereParams.bTraceComplex = false;
	SphereParams.AddIgnoredActor(EffectCauser);
	SphereParams.AddIgnoredActors(IgnoreActors);

	// query scene to see what we hit
	TArray<FOverlapResult> Overlaps;
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		World->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity, DamageChannel, FCollisionShape::MakeSphere(OuterRadius), SphereParams);
	}
	
	// collate into per-actor list of hit components
	TMap<AActor*, TArray<FHitResult> > OverlapComponentMap;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* const OverlapActor = Overlap.OverlapObjectHandle.FetchActor();

		if (OverlapActor && Overlap.Component.IsValid())
		{
			if (FHitResult Hit; ComponentIsDamageableFrom(Overlap.Component.Get(), Origin, EffectCauser, IgnoreActors, DamagePreventionChannel, Hit))
			{
				TArray<FHitResult>& HitList = OverlapComponentMap.FindOrAdd(OverlapActor);
				HitList.Add(Hit);
			}
		}
	}

	bool bAppliedDamage = false;

	if (OverlapComponentMap.Num() > 0)
	{
		// call damage function on each affected actors
		for (auto& [HitActor, HitComponents] : OverlapComponentMap)
		{
			float Damage = CalculateRadialDamage(BaseLevel, Origin, HitComponents, InnerRadius, OuterRadius, Falloff, MinimumLevel);
			ApplyGameplayEffect(Source, Cast<IAbilitySystemInterface>(HitActor), EffectClass, Damage);
			
			bAppliedDamage = true;
		}
	}

	return bAppliedDamage;
}

// note: this will automatically fall back to line test if radius is small enough
bool UShooterGameplayStatics::PredictProjectilePath(const UObject* WorldContextObject, const FPredictProjectilePathParams& PredictParams, FPredictProjectilePathResult& PredictResult)
{
	PredictResult.Reset();
	bool bBlockingHit = false;

	UWorld const* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World && PredictParams.SimFrequency > KINDA_SMALL_NUMBER)
	{
		const float SubstepDeltaTime = 1.f / PredictParams.SimFrequency;
		const float GravityZ = PredictParams.OverrideGravityZ;
		const float ProjectileRadius = PredictParams.ProjectileRadius;

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PredictProjectilePath), PredictParams.bTraceComplex);
		FCollisionObjectQueryParams ObjQueryParams;
		const bool bTraceWithObjectType = (PredictParams.ObjectTypes.Num() > 0);
		const bool bTracePath = PredictParams.bTraceWithCollision && (PredictParams.bTraceWithChannel || bTraceWithObjectType);
		if (bTracePath)
		{
			QueryParams.AddIgnoredActors(PredictParams.ActorsToIgnore);
			if (bTraceWithObjectType)
			{
				for (auto Iter = PredictParams.ObjectTypes.CreateConstIterator(); Iter; ++Iter)
				{
					const ECollisionChannel& Channel = UCollisionProfile::Get()->ConvertToCollisionChannel(false, *Iter);
					ObjQueryParams.AddObjectTypesToQuery(Channel);
				}
			}
		}

		FVector CurrentVel = PredictParams.LaunchVelocity;
		FVector TraceStart = PredictParams.StartLocation;
		FVector TraceEnd = TraceStart;
		float CurrentTime = 0.f;
		PredictResult.PathData.Reserve(FMath::Min(128, FMath::CeilToInt(PredictParams.MaxSimTime * PredictParams.SimFrequency)));
		PredictResult.AddPoint(TraceStart, CurrentVel, CurrentTime);

		FHitResult ObjectTraceHit(NoInit);
		FHitResult ChannelTraceHit(NoInit);
		ObjectTraceHit.Time = 1.f;
		ChannelTraceHit.Time = 1.f;

		const float MaxSimTime = PredictParams.MaxSimTime;
		while (CurrentTime < MaxSimTime)
		{
			// Limit step to not go further than total time.
			const float PreviousTime = CurrentTime;
			const float ActualStepDeltaTime = FMath::Min(MaxSimTime - CurrentTime, SubstepDeltaTime);
			CurrentTime += ActualStepDeltaTime;

			// Integrate (Velocity Verlet method)
			TraceStart = TraceEnd;
			FVector OldVelocity = CurrentVel;
			CurrentVel = OldVelocity + FVector(0.f, 0.f, GravityZ * ActualStepDeltaTime);
			TraceEnd = TraceStart + (OldVelocity + CurrentVel) * (0.5f * ActualStepDeltaTime);
			PredictResult.LastTraceDestination.Set(TraceEnd, CurrentVel, CurrentTime);

			if (bTracePath)
			{
				bool bObjectHit = false;
				bool bChannelHit = false;
				if (bTraceWithObjectType)
				{
					bObjectHit = World->SweepSingleByObjectType(ObjectTraceHit, TraceStart, TraceEnd, FQuat::Identity, ObjQueryParams, FCollisionShape::MakeSphere(ProjectileRadius), QueryParams);
				}
				if (PredictParams.bTraceWithChannel)
				{
					bChannelHit = World->SweepSingleByChannel(ChannelTraceHit, TraceStart, TraceEnd, FQuat::Identity, PredictParams.TraceChannel, FCollisionShape::MakeSphere(ProjectileRadius), QueryParams);
				}

				// See if there were any hits.
				if (bObjectHit || bChannelHit)
				{
					// Hit! We are done. Choose trace with earliest hit time.
					PredictResult.HitResult = (ObjectTraceHit.Time < ChannelTraceHit.Time) ? ObjectTraceHit : ChannelTraceHit;
					const float HitTimeDelta = ActualStepDeltaTime * PredictResult.HitResult.Time;
					const float TotalTimeAtHit = PreviousTime + HitTimeDelta;
					const FVector VelocityAtHit = OldVelocity + FVector(0.f, 0.f, GravityZ * HitTimeDelta);
					PredictResult.AddPoint(PredictResult.HitResult.Location, VelocityAtHit, TotalTimeAtHit);
					bBlockingHit = true;
					break;
				}
			}

			PredictResult.AddPoint(TraceEnd, CurrentVel, CurrentTime);
		}

		// Draw debug path
#if ENABLE_DRAW_DEBUG
		if (PredictParams.DrawDebugType != EDrawDebugTrace::None)
		{
			const bool bPersistent = PredictParams.DrawDebugType == EDrawDebugTrace::Persistent;
			const float LifeTime = (PredictParams.DrawDebugType == EDrawDebugTrace::ForDuration) ? PredictParams.DrawDebugTime : 0.f;
			const float DrawRadius = (ProjectileRadius > 0.f) ? ProjectileRadius : 5.f;

			// draw the path
			for (const FPredictProjectilePathPointData& PathPt : PredictResult.PathData)
			{
				::DrawDebugSphere(World, PathPt.Location, DrawRadius, 12, FColor::Green, bPersistent, LifeTime);
			}
			// draw the impact point
			if (bBlockingHit)
			{
				::DrawDebugSphere(World, PredictResult.HitResult.Location, DrawRadius + 1.0f, 12, FColor::Red, bPersistent, LifeTime);
			}
		}
#endif //ENABLE_DRAW_DEBUG
	}

	return bBlockingHit;
}

void UShooterGameplayStatics::PlayMontage(UAnimInstance* Instance, UAnimMontage* Montage, FName SectionName)
{
	if (Instance && Montage)
	{
		Instance->Montage_Play(Montage);
		if (SectionName != NAME_None)
		{
			Instance->Montage_JumpToSection(SectionName, Montage);
		}
	}
}
