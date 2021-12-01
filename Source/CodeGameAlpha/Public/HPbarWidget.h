// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HPbarWidget.generated.h"

class UProgressBar;
/**
 * 
 */
UCLASS()
class CODEGAMEALPHA_API UHPbarWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void DiedSetHPBar();
protected:
	UPROPERTY(BlueprintReadWrite,meta = (BindWidget))
	UProgressBar* HPBar;
};
