// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/OverheadWidget.h"

#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(const FString& TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	const ENetRole RemoteRole = InPawn->GetRemoteRole();
	FString Role;
	switch (RemoteRole)
	{
	case ENetRole::ROLE_Authority:
		Role = TEXT("Authority");
		break;
	case ENetRole::ROLE_AutonomousProxy:
		Role = TEXT("Autonomous Proxy");
		break;
	case ENetRole::ROLE_SimulatedProxy:
		Role = TEXT("Simulated Proxy");
		break;
	case ROLE_None:
		Role = TEXT("None");
		break;
	case ROLE_MAX:
		break;
	}
	SetDisplayText(Role);
}

void UOverheadWidget::NativeDestruct()
{
	Super::NativeDestruct();
	RemoveFromParent();
}
