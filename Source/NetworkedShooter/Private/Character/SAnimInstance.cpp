// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SAnimInstance.h"
#include "Character/SCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/SWeapon.h"

void USAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ASCharacter>(TryGetPawnOwner());
}

void USAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Character == nullptr)
	{
		Character = Cast<ASCharacter>(TryGetPawnOwner());
		if (Character == nullptr) return;
	}
	
	FVector Velocity = Character->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	if (const UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
	{
		bIsInAir = MovementComponent->IsFalling();
		bIsAccelerating = MovementComponent->GetCurrentAcceleration().Size() > 0.f ? true : false;
	}

	bWeaponEquipped = Character->IsWeaponEquipped();
	EquippedWeapon = Character->GetEquippedWeapon();
	bAiming = Character->IsAiming();
	bIsCrouched = Character->bIsCrouched;
	AO_Yaw = Character->GetAO_Yaw();
	AO_Pitch = Character->GetAO_Pitch();
	TurningInPlace = Character->GetTurningInPlace();
	bRotateRootBone = Character->ShouldRotateRootBone();
	bEliminated = Character->IsEliminated();

	// offset yaw for strafing
	const FRotator AimRotation = Character->GetBaseAimRotation();
	const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(Character->GetVelocity());
	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 6.f);
	YawOffset = DeltaRotation.Yaw;
	
	// calculate lean
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = Character->GetActorRotation();
	if (Speed > 0.0f)  // only lean if moving
	{
		const float DeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame).Yaw;
		Lean = FMath::Clamp(FMath::FInterpTo(Lean, DeltaYaw / DeltaSeconds, DeltaSeconds, 6.f), -90.f, 90.f);
		if (Character->GetActorForwardVector().Dot(Character->GetVelocity()) < 0.f) Lean = -Lean;  // lean in the opposite direction if moving backwards
	}
	else
	{
		Lean = 0.f;
	}
	
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && Character->GetMesh())
	{
		
		
		// const FVector SocketLocation = EquippedWeapon->GetWeaponMesh()->GetSocketLocation("LeftHandSocket");
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform("LeftHandSocket");
		FVector OutPosition;
		FRotator OutRotation;
		
		Character->GetMesh()->TransformToBoneSpace("hand_r",
			LeftHandTransform.GetLocation(),
			FRotator::ZeroRotator,
			OutPosition,
			OutRotation
		);

		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (Character->IsLocallyControlled())
		{
			bLocallyControlled = true;
			const FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform("hand_r");
			const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(
				RightHandTransform.GetLocation(),
				RightHandTransform.GetLocation() + RightHandTransform.GetLocation() - Character->GetHitTarget()
			);
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaSeconds, 15.f);
		}
	}

	bUseFABRIK = Character->GetCombatState() == ESCombatState::ECS_Unoccupied;
	bUseAimOffsets = Character->GetCombatState() == ESCombatState::ECS_Unoccupied && !Character->bDisableGameplay;
	bTransformRightHand = Character->GetCombatState() == ESCombatState::ECS_Unoccupied && !Character->bDisableGameplay;
}
