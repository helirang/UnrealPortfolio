// Fill out your copyright notice in the Description page of Project Settings.

#include "HPbarWidget.h"
#include "ProgressBar.h"



void UHPbarWidget::DiedSetHPBar()
{
	float ZeroPercent = 0.0f;
	HPBar->SetPercent(ZeroPercent);
}
