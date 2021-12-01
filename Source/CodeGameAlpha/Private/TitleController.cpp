// Fill out your copyright notice in the Description page of Project Settings.

#include "TitleController.h"
#include "TitleWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

ATitleController::ATitleController()
{
	static ConstructorHelpers::FClassFinder<UTitleWidget> UI_TITLE_C(TEXT("WidgetBlueprint'/Game/GameF/Tittle/WB_Tittle.WB_Tittle_C'"));
	if (UI_TITLE_C.Succeeded())
	{
		TitleWidgetClass = UI_TITLE_C.Class;
	}
}

void ATitleController::BeginPlay()
{
	Super::BeginPlay();

	TitleWidget = CreateWidget<UTitleWidget>(this, TitleWidgetClass);
	TitleWidget->AddToViewport();

	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());

	frint = "Null";
}


void ATitleController::StartGame()
{
	UGameplayStatics::OpenLevel(GetWorld(), FName("Blockout_P"), true, ((FString)(L"Listen")));
}

void ATitleController::JoinFriend(FString IPNumber)
{
	FName FrinedIp = FName(*IPNumber);
	UGameplayStatics::OpenLevel(GetWorld(), FrinedIp);
}
