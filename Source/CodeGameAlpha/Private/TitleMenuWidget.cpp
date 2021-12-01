// Fill out your copyright notice in the Description page of Project Settings.

#include "TitleMenuWidget.h"
#include "TitleController.h"

void UTitleMenuWidget::StartServer()
{
	ATitleController* TitlePC = Cast<ATitleController>(GetOwningPlayer());
	if (TitlePC)
	{
		TitlePC->StartGame();
	}
}

void UTitleMenuWidget::JoinFriendServer()
{
	ATitleController* TitlePC = Cast<ATitleController>(GetOwningPlayer());
	if (TitlePC)
	{

		TitlePC->JoinFriend(FriendIp);
		
	}
}

void UTitleMenuWidget::SetIpAddress(FString IPAddress)
{
	FriendIp = IPAddress;
}
