// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MainCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UCharacterMovementComponent;
class UMaingameHealthComponent;
class UHPbarWidget;
class AOriginWeapon;
class AController;
class UAudioComponent;

UCLASS()
class CODEGAMEALPHA_API AMainCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMainCharacter();

protected:
	virtual void BeginPlay() override;

	//캐릭터 동작 기능
	virtual void Jump() override;

	void MoveForward(float Value);
	void MoveRight(float Value);

	void BeginCrouch();
	void EndCrouch();

	//게임 종료
	void QuitUser();

	//Zoom 기능
	void BeginZoom();
	UFUNCTION(BlueprintCallable)
	void EndZoom();
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerChangeZoom();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ZoomLocationChange();

	//사운드 기능
	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void SoundPlay();
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerChangeSound(bool OnOff);
	
	//체력 변경 및 사망
	UFUNCTION()
	void OnHealthChanged(UMaingameHealthComponent* OwningHealthComp, float Health, float HealthDelta,
		const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);
	UFUNCTION()
	void OnRep_bDied();
	UFUNCTION(BlueprintCallable, Category = "Death")
	void FallDeath();
	UFUNCTION(BlueprintCallable, Category = "Death")
	void ClearDeath(float DisappearTime);

	//리스폰 기능
	UFUNCTION(BlueprintImplementableEvent)
	void ReSpawnRequest();

	//AI Hearing이 인식하는 Noise 생성 메서드
	UFUNCTION(BlueprintImplementableEvent)
	void FireAddReportNoiseEvent();

	UFUNCTION()
	void OnRep_bFire();

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual FVector GetPawnViewLocation() const override;

	//Fire BP 연동 및 비헤이비어 Call
	UFUNCTION(BlueprintCallable, Category = "Player")
	void StartFire();
	UFUNCTION(BlueprintCallable, Category = "Bot")
	void StartBotFire();
	UFUNCTION(BlueprintCallable, Category = "Player")
	void StopFire();



	//사운드 BP 연동
	UFUNCTION(BlueprintCallable, Category = "Player")
	void FireSound(bool OnOff);
	UFUNCTION(BlueprintCallable, Category = "Player")
	void StopFireSound();

	UFUNCTION(BlueprintCallable, Category = "Bot")
	float ReturnPitch();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;

	//Zoom
	float DefalutFOV;
	bool bZoomReady;
	UPROPERTY(EditDefaultsOnly, Category = "Player")
	float ZooomedFOV;
	UPROPERTY(EditDefaultsOnly, Category = "Player", meta = (ClampMin = 0.1, ClampMax = 100))
	float ZoomInterSpeed; //meta를 사용한 값 제한, 0.1 ~ 100
	UPROPERTY(BlueprintReadOnly)
	bool bZoomLocation;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
	bool bWantsToZoom;

	//Health
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UMaingameHealthComponent* HealthComp;

	//Weapon
	UPROPERTY(BlueprintReadOnly,Replicated)
	AOriginWeapon* CurrentWeapon;
	UPROPERTY(VisibleDefaultsOnly, Category = "Player")
	FName WeaponAttachSocketName;
	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<AOriginWeapon> StarterWeaponClass;
	
	//Pawn died previously
	UPROPERTY(ReplicatedUsing = OnRep_bDied, BlueprintReadOnly, Category = "Player")
	bool bDied;

	//메테리얼 WorldOffSet을 사용한 캐릭터 등장, 퇴장 기능의 트리거 역할을 하는 변수
	UPROPERTY(BlueprintReadWrite)
	bool bAppearOrDisAppear;

	// Fire
	FTimerHandle WaitHandle; // (발사)에 사용되는 타임 핸들러
	UPROPERTY(ReplicatedUsing = OnRep_bFire, BlueprintReadOnly, Category = "Player")
	bool bFire;

	//Sound
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TArray<UAudioComponent*> AudioCompArray;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Audio")
	UAudioComponent* CharacterAudioComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	UAudioComponent* CharacterAudioComp2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	UAudioComponent* CharacterAudioComp3;

	// Npc Check 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bBot;

private:
	UCharacterMovementComponent* MyMoveComp;

public:
	UPROPERTY(BlueprintReadWrite)
	float pitch2;
};
