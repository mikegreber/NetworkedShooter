// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#define ECC_SkeletalMesh ECollisionChannel::ECC_GameTraceChannel1

#define NET_ROLE_STRING_ACTOR FString(HasAuthority() ? "Server" : "Client") 
#define NET_ROLE_STRING_COMPONENT FString(GetOwner()->HasAuthority() ? "Server" : "Client")

#ifdef WITH_EDITOR
#define SERVER_ONLY() if (!GetOwner()->HasAuthority()) UE_LOG(LogTemp, Warning, TEXT("%s called on client, should only be called from server!"), __FUNCTIONW__)
#define LOCALLY_CONTROLLED_ONLY(Pawn) if (!Pawn || !Pawn->IsLocallyControlled()) UE_LOG(LogTemp, Warning, TEXT("%s called on client, should only be called locally!"), __FUNCTIONW__)
#else
#define SERVER_ONLY() {}
#define LOCALLY_CONTROLLED_ONLY() {}
#endif

template<typename T>
T ClampWithOverflow(T Value, T Min, T Max, T& OutOverflow)
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
