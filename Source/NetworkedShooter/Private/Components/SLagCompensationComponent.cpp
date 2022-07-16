// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SLagCompensationComponent.h"
#include "Character/SCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Library/ShooterGameplayStatics.h"
#include "NetworkedShooter/NetworkedShooter.h"
#include "PlayerController/SPlayerController.h"
#include "Weapon/SProjectile.h"
#include "Weapon/SProjectile_Rocket.h"
#include "Weapon/SWeapon.h"
#include "Weapon/SWeapon_Projectile.h"


FColliderInfo::FColliderInfo(const UShapeComponent* Collider)
{
	Location = Collider->GetComponentLocation();
	Rotation = Collider->GetComponentRotation();
}



USLagCompensationComponent::USLagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USLagCompensationComponent::SetOwnerCharacter(ASCharacter* NewCharacter)
{
	OwnerCharacter = NewCharacter;
}

void USLagCompensationComponent::SetPlayerController(ASPlayerController* NewPlayerController)
{
	PlayerController = NewPlayerController;
}

void USLagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority()) SetComponentTickEnabled(true);
}

void USLagCompensationComponent::ServerRewindHitTrace(const TArray<ASCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, ASWeapon* DamageCauser, int32 Seed)
{
	if (Seed) ServerRewindHitTraceWithSeed(HitCharacters, TraceStart, HitLocation, HitTime, DamageCauser, Seed);
	
	else ServerRewindHitTraceNoSeed(HitCharacters, TraceStart, HitLocation, HitTime, DamageCauser);
}

void USLagCompensationComponent::ServerRewindHitProjectile_Implementation(const TArray<ASCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart,const FVector_NetQuantize100& InitialVelocity, float HitTime, ASWeapon_Projectile* DamageCauser)
{
	TMap<ASCharacter*, FFramePackage> CurrentFrames;
	MoveCollidersToFrameAtHitTime(CurrentFrames, HitCharacters, HitTime);
	
	RewindProjectileHit(DamageCauser, TraceStart, InitialVelocity);
	
	ResetColliders(CurrentFrames);
}

void USLagCompensationComponent::ServerRewindHitTraceWithSeed_Implementation(const TArray<ASCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitTarget, float HitTime, ASWeapon* DamageCauser, int32 Seed)
{
	TMap<ASCharacter*, FFramePackage> CurrentFrames;
	MoveCollidersToFrameAtHitTime(CurrentFrames, HitCharacters, HitTime);

	// fire trace only shot
	DamageCauser->LocalFire(FTransform(TraceStart), HitTarget, true, Seed);

	ResetColliders(CurrentFrames);
}

void USLagCompensationComponent::ServerRewindHitTraceNoSeed_Implementation(const TArray<ASCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitTarget, float HitTime, ASWeapon* DamageCauser)
{
	TMap<ASCharacter*, FFramePackage> CurrentFrames;
	MoveCollidersToFrameAtHitTime(CurrentFrames, HitCharacters, HitTime);

	// fire trace only shot
	DamageCauser->LocalFire(FTransform(TraceStart), HitTarget, true);

	ResetColliders(CurrentFrames);
}

void USLagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SaveFramePackage();
}

void USLagCompensationComponent::RewindProjectileHit(ASWeapon_Projectile* DamageCauser,const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity) const
{
	ASProjectile* ProjectileCDO = Cast<ASProjectile>(DamageCauser->GetProjectileClass()->GetDefaultObject());
	if (!ProjectileCDO) return;
	
	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithChannel = true;
	PathParams.bTraceWithCollision = true;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.OverrideGravityZ = ProjectileCDO->ProjectileGravityScale;
	PathParams.MaxSimTime = 4.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.SimFrequency = 30.f;
	PathParams.StartLocation = TraceStart;
	PathParams.TraceChannel = ECC_RewindTrace;
	PathParams.ActorsToIgnore.Add(GetOwner());
	// PathParams.DrawDebugTime = 5.f;
	// PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	
	FPredictProjectilePathResult PathResult;
	if (UShooterGameplayStatics::PredictProjectilePath(this, PathParams, PathResult))
	{
		float Damage = PathResult.HitResult.GetComponent()->GetFName() == "Head" ? DamageCauser->GetHeadshotDamage() : DamageCauser->GetDamage();
		ProjectileCDO->ApplyDamage(
			this,
			PathResult.HitResult.Location,
			PathResult.HitResult.GetActor(),
			Damage,
			DamageCauser->OwnerController,
			DamageCauser,
			UDamageType::StaticClass(),
			ECC_RewindTrace
		);
	}
}

FORCEINLINE void USLagCompensationComponent::SaveFramePackage()
{
	if (FrameHistory.Num() <= 1)
	{
		FrameHistory.AddHead(GetCurrentFramePackage());
	}
	else
	{
		const float HeadTime = FrameHistory.GetHead()->GetValue().Time;
		while (HeadTime - FrameHistory.GetTail()->GetValue().Time > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
		}
		
		FrameHistory.AddHead(GetCurrentFramePackage());
	}
}

FORCEINLINE FFramePackage USLagCompensationComponent::GetCurrentFramePackage() const
{
	return FFramePackage(GetWorld()->GetTimeSeconds(), OwnerCharacter->GetRewindColliders());
}

void USLagCompensationComponent::ShowFramePackage(const FFramePackage& Package, FColor Color) const
{
	if (!OwnerCharacter) return;
		
	for (const auto& [Name, Collider] : Package.Colliders)
	{
		const UShapeComponent* Shape = OwnerCharacter->GetRewindColliders()[Name];
		if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Shape))
		{
			DrawDebugCapsule(GetWorld(), Collider.Location, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), FQuat::MakeFromRotator(Collider.Rotation), Color, false, 5);
		}
	}
}

FFramePackage USLagCompensationComponent::GetFrameToCheck(const ASCharacter* HitCharacter, float HitTime)
{
	// frame package to check to verify hit
	FFramePackage FrameToCheck{};
	
	if (HitCharacter &&
		HitCharacter->GetLagCompensationComponent() &&
		HitCharacter->GetLagCompensationComponent()->FrameHistory.GetHead() &&
		HitCharacter->GetLagCompensationComponent()->FrameHistory.GetTail())
	{
		// frame history of HitCharacter
		const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensationComponent()->FrameHistory;
		const float OldestHistoryTime = History.GetTail()->GetValue().Time;
		const float NewestHistoryTime = History.GetHead()->GetValue().Time;

		// too far back - too laggy for rewind
		if (OldestHistoryTime > HitTime)
		{
			return FrameToCheck;
		}
			
		if (OldestHistoryTime == HitTime)
		{
			FrameToCheck = History.GetTail()->GetValue();
		}
		else if (NewestHistoryTime <= HitTime)
		{
			FrameToCheck = History.GetHead()->GetValue();
		}
		else
		{
			TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* YoungerFrame = History.GetHead();
			TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* OlderFrame = YoungerFrame;

			// scan until Older.Time < HitTime < Younger.Time
			while(OlderFrame->GetValue().Time > HitTime && OlderFrame->GetNextNode())
			{
				OlderFrame = OlderFrame->GetNextNode();
				if (OlderFrame->GetValue().Time > HitTime) YoungerFrame = OlderFrame;
			}

			if (OlderFrame->GetValue().Time == HitTime)
			{
				FrameToCheck = OlderFrame->GetValue();
			}
			else
			{
				FrameToCheck = InterpBetweenFrames(OlderFrame->GetValue(), YoungerFrame->GetValue(), HitTime);
			}
		}
	}
	return FrameToCheck;
}

FFramePackage USLagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = (HitTime - OlderFrame.Time) / Distance;

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	for (const auto& [Name, YoungerCapsule] : YoungerFrame.Colliders)
	{
		const FColliderInfo& OlderCapsule = OlderFrame.Colliders[Name];

		FColliderInfo InterpCapsuleInfo;
		InterpCapsuleInfo.Location = FMath::VInterpTo(OlderCapsule.Location, YoungerCapsule.Location, 1.f, InterpFraction);
		InterpCapsuleInfo.Rotation = FMath::RInterpTo(OlderCapsule.Rotation, YoungerCapsule.Rotation, 1.f, InterpFraction);
		InterpFramePackage.Colliders.Emplace(Name, InterpCapsuleInfo);
	}

	return InterpFramePackage;
}

void USLagCompensationComponent::MoveCollidersToFrameAtHitTime(TMap<ASCharacter*, FFramePackage>& CurrentFrames, const TArray<ASCharacter*>& HitCharacters, float HitTime) const
{
	for (auto HitCharacter : HitCharacters)
	{
		if (HitCharacter)
		{
			// cache current collider positions so we can move them back
			CurrentFrames.Add(HitCharacter, FFramePackage(GetWorld()->GetTimeSeconds(), HitCharacter->GetRewindColliders()));

			FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
			
			// move capsules to position at frame
			for (auto& [Name, Capsule] : HitCharacter->GetRewindColliders())
			{
				Capsule->SetWorldLocation(FrameToCheck.Colliders[Name].Location);
				Capsule->SetWorldRotation(FrameToCheck.Colliders[Name].Rotation);
				Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}
			HitCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void USLagCompensationComponent::ResetColliders(TMap<ASCharacter*, FFramePackage>& CurrentFrames)
{
	for (auto& [HitCharacter, Frame] : CurrentFrames)
	{
		if (HitCharacter)
		{
			// disable capsule collisions and return to attached positions
			for (auto& [Name, Capsule] : HitCharacter->GetRewindColliders())
			{
				Capsule->SetWorldLocation(Frame.Colliders[Name].Location);
				Capsule->SetWorldRotation(Frame.Colliders[Name].Rotation);
				Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
			HitCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
	}
}