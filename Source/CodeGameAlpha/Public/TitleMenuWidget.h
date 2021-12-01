// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitleMenuWidget.generated.h"


/**
 * 
 */
UCLASS()
class CODEGAMEALPHA_API UTitleMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = UI)
	void StartServer();

	UFUNCTION(BlueprintCallable, Category = UI)
	void JoinFriendServer();

	UFUNCTION(BlueprintCallable, Category = UI)
	void SetIpAddress(FString IPAddress);

private:
	FString FriendIp;
};
