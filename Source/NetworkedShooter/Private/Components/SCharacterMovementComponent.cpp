// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SCharacterMovementComponent.h"

#include "AbilitySystem/SAttributeSet.h"
#include "GameFramework/Character.h"

void USCharacterMovementComponent::FSSavedMove::Clear()
{
	FSavedMove_Character::Clear();
	
	SavedRequestToStartSprinting = false;
	SavedRequestToStartADS = false;
}

void USCharacterMovementComponent::FSSavedMove::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel,FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	if (const USCharacterMovementComponent* CharacterMovement = Cast<USCharacterMovementComponent>(Character->GetCharacterMovement()))
	{
		SavedRequestToStartSprinting = CharacterMovement->RequestToStartSprinting;
		SavedRequestToStartADS = CharacterMovement->RequestToStartADS;
	}
}

bool USCharacterMovementComponent::FSSavedMove::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
	if (SavedRequestToStartSprinting != ((FSSavedMove*) &NewMove)->SavedRequestToStartSprinting) return false;

	if (SavedRequestToStartADS != ((FSSavedMove*) &NewMove)->SavedRequestToStartADS) return false;
	
	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void USCharacterMovementComponent::FSSavedMove::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);
}

uint8 USCharacterMovementComponent::FSSavedMove::GetCompressedFlags() const
{
	

	uint8 Result = Super::GetCompressedFlags();

	if (SavedRequestToStartSprinting) Result |= FLAG_Custom_0;
	
	if (SavedRequestToStartADS) Result |= FLAG_Custom_1;
	
	return Result;
}

USCharacterMovementComponent::FSNetworkPredictionData_Client::FSNetworkPredictionData_Client(const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement)
{
}

FSavedMovePtr USCharacterMovementComponent::FSNetworkPredictionData_Client::AllocateNewMove()
{
	return FSavedMovePtr(new FSSavedMove());
}

USCharacterMovementComponent::USCharacterMovementComponent()
{
	SprintSpeed = 2000;
	ADSSpeed = 450;
}

void USCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	RequestToStartSprinting = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;

	RequestToStartADS = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
}

FNetworkPredictionData_Client* USCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner);

	if (!ClientPredictionData)
	{
		USCharacterMovementComponent* MutableThis = const_cast<USCharacterMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FSNetworkPredictionData_Client(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}

	return ClientPredictionData;
}

void USCharacterMovementComponent::StartSprinting()
{
	RequestToStartSprinting = true;
}

void USCharacterMovementComponent::StopSprinting()
{
	RequestToStartSprinting = false;
}


void USCharacterMovementComponent::StartADS()
{
	RequestToStartADS = true;
}

void USCharacterMovementComponent::StopADS()
{
	RequestToStartADS = false;
}

float USCharacterMovementComponent::GetMaxSpeed() const
{
	if (RequestToStartSprinting)
	{
		return AttributeSet->GetSprintSpeed();
	}
	
	if (RequestToStartADS)
	{
		return AttributeSet->GetADSSpeed();
	}
	
	return AttributeSet->GetSpeed();
}


