// Fill out your copyright notice in the Description page of Project Settings.

#include "MaingameHealthComponent.h"
#include "Net/UnrealNetwork.h"

static int32 DebugHealthCompDrawing = 0;
FAutoConsoleVariableRef CVARDebugHealthCompDrawing(
	TEXT("MainGame.DebugHealtComp"),
	DebugHealthCompDrawing,
	TEXT("Draw Debug Lines for HealtComp"),
	ECVF_Cheat); //콘솔변수

UMaingameHealthComponent::UMaingameHealthComponent()
{
	DefaultHealth = 100;
	bIsDead = false;

	TeamNum = 255;

	SetIsReplicated(true);
}


void UMaingameHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority) //서버가 체력 계산
	{
		AActor* MyOwner = GetOwner();
		if (MyOwner)
		{
			MyOwner->OnTakeAnyDamage.AddDynamic(this, &UMaingameHealthComponent::HandleTakeAnyDamage);
			//OnTakeAnyDamage -> (FTakeAnyDamageSignature 멀티캐스트 델리게이트에 바인딩)
		}

		Health = DefaultHealth;
	}
}

float UMaingameHealthComponent::GetHealth() const
{
	return Health;
}

void UMaingameHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, 
	const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f || bIsDead)
	{
		return;
	}

	if (DamageCauser != DamagedActor && IsFriendly(DamagedActor, DamageCauser))
	{
		return;
	}

	//Update health Clamp ( Health == 0.0 <= Clamp <= DefaultHealth; )
	Health = FMath::Clamp(Health - Damage, 0.0f, DefaultHealth);

	if (DebugHealthCompDrawing > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Health Changed: %s"), *FString::SanitizeFloat(Health));
	}

	OnHealthChanged.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);

	bIsDead = Health <= 0.0f;

	// Widget이 값을 읽기 전에 아래 구문에서 서버의 Pawn을 빨리 죽여버려서 에러가 뜨는 것 같다.
	// 서버 유저의 컨트롤러를 사용해서 위젯을 변경하는 방식으로 해결
}

void UMaingameHealthComponent::Heal(float HealAmount)
{
	if (HealAmount <= 0.0f || Health <= 0.0f)
	{
		return;
	}

	Health = FMath::Clamp(Health + HealAmount, 0.0f, DefaultHealth);

	if (DebugHealthCompDrawing > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Health Changed: %s (+%s)"), *FString::SanitizeFloat(Health), *FString::SanitizeFloat(HealAmount));
	}

	OnHealthChanged.Broadcast(this, Health, -HealAmount, nullptr, nullptr, nullptr);
}

bool UMaingameHealthComponent::IsFriendly(AActor* ActorA, AActor* ActorB)
{
	if (ActorA == nullptr || ActorB == nullptr)
	{
		//Assume Friendly
		return true;
	}
	UMaingameHealthComponent* HealthCompA = Cast<UMaingameHealthComponent>(ActorA->GetComponentByClass(UMaingameHealthComponent::StaticClass()));
	UMaingameHealthComponent* HealthCompB = Cast<UMaingameHealthComponent>(ActorB->GetComponentByClass(UMaingameHealthComponent::StaticClass()));

	if (HealthCompA == nullptr || HealthCompB == nullptr)
	{
		//Assume Friendly
		return true;
	}

	return HealthCompA->TeamNum == HealthCompB->TeamNum;
}

void UMaingameHealthComponent::OnRep_Health(float OldHealth)
{
	float Damage = Health - OldHealth;
	if (DebugHealthCompDrawing > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Health = %f"), Health);
		UE_LOG(LogTemp, Log, TEXT("OldHealth = %f"), OldHealth);
		UE_LOG(LogTemp, Log, TEXT("Damage = %f"), Damage);
	}
	
	OnHealthChanged.Broadcast(this, Health, Damage, nullptr, nullptr, nullptr);
}

void UMaingameHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMaingameHealthComponent, Health);
}