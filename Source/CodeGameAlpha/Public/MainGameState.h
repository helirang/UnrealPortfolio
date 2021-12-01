// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MainGameState.generated.h"

UENUM(BlueprintType) 
enum class EWaveState : uint8
{
	WatingToStart,
	WaveInProgress,
	WatingToComplete,
	WaveComplete,
	GameOver,
};

/**
 * 
 */
UCLASS()
class CODEGAMEALPHA_API AMainGameState : public AGameStateBase
{
	GENERATED_BODY()
	
protected:
	UFUNCTION()
	void OnRep_WaveState(EWaveState OldState);
	UFUNCTION(BlueprintImplementableEvent, Category = "GameState")
	void WaveStateChange(EWaveState NewState, EWaveState OldState);

public:
	void SetWaveState(EWaveState NewState);
	
protected:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WaveState, Category = "GameState")
	EWaveState WaveState;
};
