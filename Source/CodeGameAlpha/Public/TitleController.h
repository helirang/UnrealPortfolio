// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TitleController.generated.h"

class USoundCue;

/**
 * 
 */
UCLASS()
class CODEGAMEALPHA_API ATitleController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ATitleController();
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = UI)
	TSubclassOf<class UTitleWidget> TitleWidgetClass;

	void StartGame();
	void JoinFriend(FString IPNumber);
	FString frint;

private:
	UPROPERTY()
	class UTitleWidget* TitleWidget;
	
	
};
