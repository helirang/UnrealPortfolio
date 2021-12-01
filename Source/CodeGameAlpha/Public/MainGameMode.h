// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainGameMode.generated.h"

class ACameraActor;
class APawn;

/**
 * 
 */
UCLASS()
class CODEGAMEALPHA_API AMainGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainGameMode();

protected:
	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void RestartDeadPlayers(APlayerController* Pc1);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void GameEnding(APlayerController* PC, ACameraActor* CA);
	UFUNCTION(BlueprintCallable)
	void EndingStart(ACameraActor* CA);
};
