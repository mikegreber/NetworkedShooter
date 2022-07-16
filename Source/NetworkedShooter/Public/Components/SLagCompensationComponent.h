// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SLagCompensationComponent.generated.h"

USTRUCT(BlueprintType)
struct FColliderInfo
{
	GENERATED_BODY()

	FColliderInfo() = default;

	FColliderInfo(const UShapeComponent* Collider);

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	FFramePackage() = default;
	FFramePackage(float FrameTime, const TMap<FName, UShapeComponent*>& ColliderComponents)
	{
		Time = FrameTime;
		
		for (const auto& [Name, Collider] : ColliderComponents)
		{
			Colliders.Add(Name, FColliderInfo(Collider));
		}
	}
	
	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FColliderInfo> Colliders;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bHeadShot;
};




UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class NETWORKEDSHOOTER_API USLagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Lag Compensation")
	float MaxRecordTime = 4.f;

	UPROPERTY() class ASCharacter* OwnerCharacter;
	UPROPERTY() class ASPlayerController* PlayerController;
	TDoubleLinkedList<FFramePackage> FrameHistory;
	
public:	
	USLagCompensationComponent();

	void SetOwnerCharacter(ASCharacter* NewCharacter);
	
	void SetPlayerController(ASPlayerController* NewPlayerController);;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Server side rewind to confirm hits
	void ServerRewindHitTrace(const TArray<ASCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, class ASWeapon* DamageCauser, int32 Seed = 0);

	UFUNCTION(Server, Reliable)
	void ServerRewindHitProjectile(const TArray<ASCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime, class ASWeapon_Projectile* DamageCauser);
	
protected:
	
	virtual void BeginPlay() override;

private:

	UFUNCTION(Server, Reliable)
	void ServerRewindHitTraceNoSeed(const TArray<ASCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, ASWeapon* DamageCauser);

	UFUNCTION(Server, Reliable)
	void ServerRewindHitTraceWithSeed(const TArray<ASCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, ASWeapon* DamageCauser, int32 Seed);

	void RewindProjectileHit(ASWeapon_Projectile* DamageCauser, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity) const;
	
	void SaveFramePackage();

	FFramePackage GetCurrentFramePackage() const;

	void ShowFramePackage(const FFramePackage& Package, FColor Color) const;
	
	void MoveCollidersToFrameAtHitTime(TMap<ASCharacter*, FFramePackage>& CurrentFrames, const TArray<ASCharacter*>& HitCharacters, float HitTime) const;

	static FFramePackage GetFrameToCheck(const ASCharacter* HitCharacter, float HitTime);
	
	static FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);

	static void ResetColliders(TMap<ASCharacter*, FFramePackage>& CurrentFrames);
};
