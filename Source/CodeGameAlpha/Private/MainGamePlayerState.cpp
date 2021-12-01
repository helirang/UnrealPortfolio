// Fill out your copyright notice in the Description page of Project Settings.

#include "MainGamePlayerState.h"

int32 AMainGamePlayerState::NowDeadScore()
{
	return DeadScore;
}

void AMainGamePlayerState::AddDeadScore()
{
	DeadScore = DeadScore + 1;
}

