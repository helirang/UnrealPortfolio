// Fill out your copyright notice in the Description page of Project Settings.

#include "MainGameMode.h"
#include "Camera/CameraActor.h"


AMainGameMode::AMainGameMode()
{

}

void AMainGameMode::EndingStart(ACameraActor* CA)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		GameEnding(PC, CA);
	}
}



