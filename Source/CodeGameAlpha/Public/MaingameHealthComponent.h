// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MaingameHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnHealthChangedSignature, UMaingameHealthComponent*, HealthComp,float, Health, float, HealthDelta,
const class UDamageType*, DamageType, class AController*, InstigatedBy, AActor*, DamageCauser);
// 1.멀티캐스트델리게이트 이름 / 2부터 매개변수 ( 2.트리거한 상태 구성 요소, 3. 체력 변수, 4.체력 변경용 변수 )
// 5부터 OnTakeAnyDamage와 연관(Actor.h), 해당 함수가 요구하는 매개변수 값 
// DoIt

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CODEGAMEALPHA_API UMaingameHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMaingameHealthComponent();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_Health(float OldHealth);

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, 
	class AController* InstigatedBy, AActor* DamageCauser);


public:	
	float GetHealth() const;
	UFUNCTION(BlueprintCallable, Category = "HealthComponent")
	void Heal(float HealAmount);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HealthComponent")
	static bool IsFriendly(AActor* ActorA, AActor* ActorB);
		
protected:
	bool bIsDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthComponent")
	float DefaultHealth;
	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "HealthComponent")
	float Health;

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChangedSignature OnHealthChanged;
	UPROPERTY(EditDefaultsOnly, Replicated, BlueprintReadOnly, Category = "HealthComponent")
	uint8 TeamNum;
};
