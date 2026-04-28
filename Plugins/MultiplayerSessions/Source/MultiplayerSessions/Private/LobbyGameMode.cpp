// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (GameState)
	{
		const int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
		GEngine->AddOnScreenDebugMessage(1,60.0f, FColor::Yellow,FString::Printf(TEXT("玩家数量%d"),NumberOfPlayers));
		TObjectPtr<APlayerState> PlayerState = NewPlayer->GetPlayerState<APlayerState>();
		if (PlayerState)
		{
			FString PlayerName = PlayerState->GetPlayerName();
			GEngine->AddOnScreenDebugMessage(1,60.0f, FColor::Cyan,FString::Printf(TEXT("玩家%s加入"),*PlayerName));
		}
		if (NumberOfPlayers == 2)
		{
			TObjectPtr<UWorld> World = GetWorld();
			if (World)
			{
				bUseSeamlessTravel = true;
				World->ServerTravel(FString("/Game/Maps/BlasterMap?listen"));
			}
		}
	}
}

void ALobbyGameMode::Logout(AController* ExitingController)
{
	Super::Logout(ExitingController);
}
