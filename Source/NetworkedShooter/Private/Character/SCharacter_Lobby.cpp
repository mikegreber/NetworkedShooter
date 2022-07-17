// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SCharacter_Lobby.h"

#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "HUD/SOverheadWidget.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerState/SPlayerState_Lobby.h"

ASCharacter_Lobby::ASCharacter_Lobby()
{
	PrimaryActorTick.bCanEverTick = false;
	
	ViewCamera = CreateDefaultSubobject<UCameraComponent>("ViewCamera");

	TopOverheadWidget = CreateDefaultSubobject<UWidgetComponent>("TopOverheadWidget");
	TopOverheadWidget->SetupAttachment(RootComponent);
	TopOverheadWidget->SetWidgetSpace(EWidgetSpace::Screen);
	TopOverheadWidget->SetDrawAtDesiredSize(true);

	MiddleOverheadWidget = CreateDefaultSubobject<UWidgetComponent>("MiddleOverheadWidget");
	MiddleOverheadWidget->SetupAttachment(RootComponent);
	MiddleOverheadWidget->SetWidgetSpace(EWidgetSpace::Screen);
	MiddleOverheadWidget->SetDrawAtDesiredSize(true);
	
	BottomOverheadWidget = CreateDefaultSubobject<UWidgetComponent>("BottomOverheadWidget");
	BottomOverheadWidget->SetupAttachment(RootComponent);
	BottomOverheadWidget->SetWidgetSpace(EWidgetSpace::Screen);
	BottomOverheadWidget->SetDrawAtDesiredSize(true);
}

void ASCharacter_Lobby::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	SetPlayerState(NewController->GetPlayerState<APlayerState>());
}

void ASCharacter_Lobby::OnRep_Controller()
{
	Super::OnRep_Controller();
	SetPlayerState(Controller->GetPlayerState<APlayerState>());
}

void ASCharacter_Lobby::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	SetPlayerState(GetPlayerState());
}

void ASCharacter_Lobby::BeginPlay()
{
	Super::BeginPlay();

	if (const AActor* CameraViewActor = UGameplayStatics::GetActorOfClass(this, CameraViewActorClass))
	{
		ViewCamera->SetWorldTransform(CameraViewActor->GetTransform());
	}

	SetBottomOverheadText("Not Ready", FColor::Red);
	SetLobbyPlayerState(GetPlayerState());
}

void ASCharacter_Lobby::SetLobbyPlayerState(APlayerState* NewPlayerState)
{
	ASPlayerState_Lobby* NewLobbyPlayerState = Cast<ASPlayerState_Lobby>(NewPlayerState);
	if (NewLobbyPlayerState && NewLobbyPlayerState != LobbyPlayerState)
	{
		LobbyPlayerState = NewLobbyPlayerState;
		if (HasAuthority() && IsLocallyControlled() || !HasAuthority() && !IsLocallyControlled())
		{
			SetTopOverheadText("Host", FColor::Yellow);
		}
		SetMiddleOverheadText(LobbyPlayerState->GetPlayerName());
		SetBottomOverheadText(LobbyPlayerState->IsReady() ? "Ready" : "Not Ready", LobbyPlayerState->IsReady() ? FColor::Green : FColor::Red);
	}
	else
	{
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(
			Handle,
			FTimerDelegate::CreateLambda([&](){ SetLobbyPlayerState(GetPlayerState()); }),
			3.f,
			false
		);
	}
}


void ASCharacter_Lobby::SetTopOverheadText(FString Text, FColor Color)
{
	if (USOverheadWidget* Widget = Cast<USOverheadWidget>(TopOverheadWidget->GetWidget()))
	{
		Widget->SetDisplayText(Text, Color);
	}
}

void ASCharacter_Lobby::SetMiddleOverheadText(FString Text, FColor Color)
{
	if (USOverheadWidget* Widget = Cast<USOverheadWidget>(MiddleOverheadWidget->GetWidget()))
	{
		Widget->SetDisplayText(Text, Color);
	}
}

void ASCharacter_Lobby::SetBottomOverheadText(FString Text, FColor Color)
{
	if (USOverheadWidget* Widget = Cast<USOverheadWidget>(BottomOverheadWidget->GetWidget()))
	{
		Widget->SetDisplayText(Text, Color);
	}
}
