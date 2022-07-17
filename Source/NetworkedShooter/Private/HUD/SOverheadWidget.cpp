// Fill out your copyright notice in the Description page of Project Settings.

#include "HUD/SOverheadWidget.h"

#include "Components/TextBlock.h"

void USOverheadWidget::SetDisplayText(FString TextToDisplay, FColor Color)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
		DisplayText->SetColorAndOpacity(FSlateColor(Color));
	}
}

void USOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	// FString LocalRole;
	// switch(InPawn->GetLocalRole())
	// {
	// case ROLE_None:
	// 	{
	// 		LocalRole = "None";
	// 		break;
	// 	}
	// case ROLE_SimulatedProxy:
	// 	{
	// 		LocalRole = "Simulated Proxy";
	// 		break;
	// 	}
	// case ROLE_AutonomousProxy:
	// 	{
	// 		LocalRole = "Autonomous Proxy";
	// 		break;
	// 	}
	// case ROLE_Authority:
	// 	{
	// 		LocalRole = "Authority";
	// 		break;
	// 	}
	// default:;
	// }
	//
	// FString RemoteRole;
	//
	// switch(InPawn->GetRemoteRole())
	// {
	// case ROLE_None:
	// 	{
	// 		RemoteRole = "None";
	// 		break;
	// 	}
	// case ROLE_SimulatedProxy:
	// 	{
	// 		RemoteRole = "Simulated Proxy";
	// 		break;
	// 	}
	// case ROLE_AutonomousProxy:
	// 	{
	// 		RemoteRole = "Autonomous Proxy";
	// 		break;
	// 	}
	// case ROLE_Authority:
	// 	{
	// 		RemoteRole = "Authority";
	// 		break;
	// 	}
	// default:;
	// }
	//
	// const FString LocalRoleString = FString::Printf(TEXT("Local Role: %s\nRemote Role: %s"), *LocalRole, *RemoteRole);
	// SetDisplayText(LocalRoleString);
}

void USOverheadWidget::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	RemoveFromParent();
	Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}
