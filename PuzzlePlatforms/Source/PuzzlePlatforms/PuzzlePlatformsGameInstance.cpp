// Fill out your copyright notice in the Description page of Project Settings.

#include "PuzzlePlatformsGameInstance.h"

#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"
#include "OnlineSessionSettings.h"

#include "PlatformTrigger.h"
#include "MenuSystem/MainMenu.h"
#include "MenuSystem/MenuWidget.h"

const static FName SESSION_NAME = TEXT("My Session Game");

UPuzzlePlatformsGameInstance::UPuzzlePlatformsGameInstance(const FObjectInitializer & ObjectInitializer)
{
	ConstructorHelpers::FClassFinder<UUserWidget> MenuBPClass(TEXT("/Game/MenuSystem/WBP_MainMenu"));
	if (!ensure(MenuBPClass.Class != nullptr)) return;

	MenuClass = MenuBPClass.Class;

	ConstructorHelpers::FClassFinder<UUserWidget> InGameMenuBPClass(TEXT("/Game/MenuSystem/WBP_InGameMenu"));
	if (!ensure(InGameMenuBPClass.Class != nullptr)) return;

	InGameMenuClass = InGameMenuBPClass.Class;
}



void UPuzzlePlatformsGameInstance::Init()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if(Subsystem != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Found Subsystem %s"), *Subsystem->GetSubsystemName().ToString());
		SessionInterface = Subsystem->GetSessionInterface();
		if(SessionInterface.IsValid())
		{
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UPuzzlePlatformsGameInstance::OnCreateSessionComplete);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UPuzzlePlatformsGameInstance::OnDestroySessionComplete);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UPuzzlePlatformsGameInstance::OnFindSessionComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UPuzzlePlatformsGameInstance::OnJoinSessionComplete);
			
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Found no subsystem"));
	}
}


//Load Menu
void UPuzzlePlatformsGameInstance::LoadMenu()
{
	if (!ensure(MenuClass != nullptr)) return;

	Menu = CreateWidget<UMainMenu>(this, MenuClass);
	if (!ensure(Menu != nullptr)) return;

	Menu->Setup();

	Menu->SetMenuInterface(this);
}


//In Game Load Menu
void UPuzzlePlatformsGameInstance::InGameLoadMenu()
{
	if (!ensure(InGameMenuClass != nullptr)) return;

	UMenuWidget* Menu = CreateWidget<UMenuWidget>(this, InGameMenuClass);
	if (!ensure(Menu != nullptr)) return;

	Menu->Setup();

	Menu->SetMenuInterface(this);
}


//Host
void UPuzzlePlatformsGameInstance::Host()
{
	if(SessionInterface.IsValid())
	{
		auto ExistingSession = SessionInterface->GetNamedSession(SESSION_NAME);
		//If there is a session, destroy it, else, Create session.
		if(ExistingSession != nullptr)
		{
			SessionInterface->DestroySession(SESSION_NAME);
		}
		else
		{
			CreateSession();
		}
		
	}
}


//Completed Create Session
void UPuzzlePlatformsGameInstance::OnCreateSessionComplete(FName SessionName, bool Success)
{
	if(!Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not Create session"));
		return;
	}
	if (Menu != nullptr)
	{
		Menu->Teardown();
	}

	UEngine* Engine = GetEngine();
	if (!ensure(Engine != nullptr)) return;

	Engine->AddOnScreenDebugMessage(0, 2, FColor::Green, TEXT("Hosting"));

	UWorld* World = GetWorld();
	if (!ensure(World != nullptr)) return;

	World->ServerTravel("/Game/ThirdPersonCPP/Maps/ThirdPersonExampleMap?listen");
}


//Refresh Server List
void UPuzzlePlatformsGameInstance::RefreshServerList()
{
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	if(SessionSearch.IsValid())
	{
		//SessionSearch->bIsLanQuery = true;
		UE_LOG(LogTemp, Warning, TEXT("Starting Find Session"));
		SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	}
}


//Completed Finding Sessions
void UPuzzlePlatformsGameInstance::OnFindSessionComplete(bool Success)
{
	if(Success && SessionSearch.IsValid() && Menu != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Finished Find Session"));

		TArray<FString> ServerNames;

		for(const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
		{
			UE_LOG(LogTemp, Error, TEXT("Found Session: %s"), *SearchResult.GetSessionIdStr());	
			ServerNames.Add(SearchResult.GetSessionIdStr());		
		}

		Menu->SetServerList(ServerNames);

	}
}


//Completed Destroy Session 
void UPuzzlePlatformsGameInstance::OnDestroySessionComplete(FName SessionName, bool Success)
{
	if(Success)
	{
		CreateSession();
	}
}


//Create a Session
void UPuzzlePlatformsGameInstance::CreateSession()
{
	if(SessionInterface.IsValid())
	{
		//Set up session settings
		FOnlineSessionSettings SessionSettings;
		SessionSettings.bIsLANMatch = true;
		SessionSettings.NumPublicConnections = 2;
		SessionSettings.bShouldAdvertise = true;

		SessionInterface->CreateSession(0, SESSION_NAME, SessionSettings);
	}
	
}


//Join a session 
void UPuzzlePlatformsGameInstance::Join(uint32 Index)
{

	if(!SessionInterface.IsValid()) return;
	if(!SessionSearch.IsValid()) return;

	if (Menu != nullptr)
	{
		Menu->Teardown();
	}

	SessionInterface->JoinSession(0, SESSION_NAME, SessionSearch->SearchResults[Index]);

	
}


//Completed Join Session 
void UPuzzlePlatformsGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if(!SessionInterface.IsValid()) return;

	FString Address;
	if(!SessionInterface->GetResolvedConnectString(SessionName, Address))
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not get connect String."));
		return;
	}

	UEngine* Engine = GetEngine();
	if (!ensure(Engine != nullptr)) return;

	Engine->AddOnScreenDebugMessage(0, 5, FColor::Green, FString::Printf(TEXT("Joining %s"), *Address));

	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!ensure(PlayerController != nullptr)) return;

	PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
}


//Load main menu
void UPuzzlePlatformsGameInstance::LoadMainMenu()
{
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!ensure(PlayerController != nullptr)) return;

	PlayerController->ClientTravel("/Game/MenuSystem/MainMenu", ETravelType::TRAVEL_Absolute);
}




