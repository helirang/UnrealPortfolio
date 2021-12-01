// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MainGamePlayerState.generated.h"

/**
 * 
 */
UCLASS()
class CODEGAMEALPHA_API AMainGamePlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "PlayerState")
	void ClientRequest(APlayerController* DeadPlayer);

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	int32 NowDeadScore();

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void AddDeadScore();

protected:
	int32 DeadScore;
};
