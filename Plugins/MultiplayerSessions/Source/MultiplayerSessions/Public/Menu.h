// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

class UMultiplayerSessionsSubsystem;
class UButton;

UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="MultiplayerSessions|Menu")
	void MenuSetup(const int32 InNumPublicConnections = 4, const FString InMatchType = FString(TEXT("FreeForAll")), const FString InLobbyPath = FString(TEXT("/Game/ThirdPerson/Lobby")));
	
protected:
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;
	
	
	UFUNCTION() //动态多播的回调函数必须加UFUNCTION宏
	void OnCreateSession(bool bSuccess);
	void OnFindSession(const TArray<FOnlineSessionSearchResult>& SessionSearchResults, bool bSuccess) const;
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
	UFUNCTION()
	void OnStartSession(bool bSuccess);
	UFUNCTION()
	void OnDestroySession(bool bSuccess);
	
private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> HostButton;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> JoinButton;
	
	UFUNCTION()
	void HostButtonClicked();
	UFUNCTION()
	void JoinButtonClicked();
	
	void RemoveMenu();
	
	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;
	
	int32 NumPublicConnections = 4;
	FString MatchType = FString(TEXT("FreeForAll"));
	FString LobbyPath = FString(TEXT(""));
};
