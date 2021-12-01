// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OriginWeapon.generated.h"

class UDamageType;
class UParticleSystem;
class USkeletalMeshComponent;

USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()

	public:
	UPROPERTY()
	TEnumAsByte<EPhysicalSurface> Surfacetype; //프로젝트 셋팅 Engine-Physics -> EPhysicalSurface

	UPROPERTY()
	FVector_NetQuantize TraceTo;
};

UCLASS()
class CODEGAMEALPHA_API AOriginWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AOriginWeapon();

protected:
	virtual void BeginPlay() override;

	//Fire
	virtual void Fire();
	virtual void Fire(FVector StartPosition, FVector EndPosition);
	virtual void BotFire();

	void TestBotFire();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	float GetBotFireDirection();

	void PlayFireEffects(FVector TracerEnd);
	void PlayImpactEffect(EPhysicalSurface SurfaceType, FVector ImpactPoint);
	FVector GetDirection(FVector StartLocation, FVector EndLocation);

	//Net
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(FVector StartPosition, FVector EndPosition);
	UFUNCTION()
	void OnRep_HitScanTrace();
	UFUNCTION(BlueprintImplementableEvent)
	void ReportNoiseImpactPoint(FVector NoiseLocation);

public:	
	//FireTrigger
	void StartFire();
	void StartBotFire();

	//StopFire
	void StopFire();
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComp;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName ButtstockSocketName;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleEffectSocketName;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName TracerTargetName;

	//Effect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* DefaultImpactEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* FleshImpactEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* MuzzleEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* TracerEffect;

	//Damage
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float BaseDamage;

	//FireShots
	float LastFireTime;
	float TimeBetweenShots;
	FTimerHandle TimerHandle_TimeBetweenShots;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 0.0f))
	float BulletSpread;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float RateOfFire;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UCameraShake> FireCamShake;

	//Fire
	FVector MuzzleLocation;
	FVector ButtstockLocation;
	FVector AimLocation;

	//ReplicatedUsing
	UPROPERTY(ReplicatedUsing = OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

	//testBotFire_Dedicate
	UPROPERTY(BlueprintReadWrite)
	AActor* TargetActor;

public:
	UPROPERTY(Replicated)
	bool bIsBot;
};
