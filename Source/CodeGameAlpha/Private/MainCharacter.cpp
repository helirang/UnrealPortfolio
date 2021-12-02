// Fill out your copyright notice in the Description page of Project Settings.

#include "MainCharacter.h"
#include "Camera/CameraComponent.h"
#include "CodeGameAlpha.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h" //이거 없어도됨
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MaingameHealthComponent.h"
#include "MainGamePlayerState.h"
#include "Net/UnrealNetwork.h"
#include "OriginWeapon.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"

AMainCharacter::AMainCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	HealthComp = CreateDefaultSubobject<UMaingameHealthComponent>(TEXT("HealthComp"));

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	ZooomedFOV = 65.0f; // 65.0f
	ZoomInterSpeed = 20.0f;

	WeaponAttachSocketName = "WeaponSocket";

	bAppearOrDisAppear = true;
	bZoomLocation = false;

	//audio
	CharacterAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("CharacterAudioCompEdit"));
	CharacterAudioComp->bAutoActivate = false;
	CharacterAudioComp->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	CharacterAudioComp->SetupAttachment(RootComponent);

	CharacterAudioComp2 = CreateDefaultSubobject<UAudioComponent>(TEXT("CharacterAudioCompEdit2"));
	CharacterAudioComp2->bAutoActivate = false;
	CharacterAudioComp2->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	CharacterAudioComp2->SetupAttachment(RootComponent);

	CharacterAudioComp3 = CreateDefaultSubobject<UAudioComponent>(TEXT("CharacterAudioCompEdit3"));
	CharacterAudioComp3->bAutoActivate = false;
	CharacterAudioComp3->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	CharacterAudioComp3->SetupAttachment(RootComponent);

	bFire = false;

	MyMoveComp = GetCharacterMovement();
}

void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefalutFOV = CameraComp->FieldOfView; //DefalutFov Setting
	HealthComp->OnHealthChanged.AddDynamic(this, &AMainCharacter::OnHealthChanged);

	if (Role == ROLE_Authority) 
	{
		FActorSpawnParameters SpawnParams; 
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; //충돌이 발생해도 계속 스폰

		CurrentWeapon = GetWorld()->SpawnActor<AOriginWeapon>(StarterWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (CurrentWeapon)
		{
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);
			if (bBot)
			{
				CurrentWeapon->bIsBot = true;
			}
		}
	}
}

void AMainCharacter::Jump()
{
	Super::Jump();

	if (!bBot && !(MyMoveComp->IsFalling()) && !(MyMoveComp->IsCrouching()))
	{
		UCharacterMovementComponent* Temp = GetCharacterMovement();
		Temp->IsCrouching();
		CharacterAudioComp2->SetIntParameter(FName("CharacterSound"), 0);
		CharacterAudioComp2->Play();
	}
}

void AMainCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector() * Value);
}

void AMainCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector()*Value);
}

void AMainCharacter::BeginCrouch()
{
	Crouch();
}

void AMainCharacter::EndCrouch()
{
	UnCrouch();
}


void AMainCharacter::BeginZoom()
{
	if (!bWasJumping)
	{
		ServerChangeZoom();
		
		bZoomLocation = true;
	}
	else
	{
		bZoomReady = true;
	}
}

void AMainCharacter::EndZoom()
{
	if (bZoomReady)
	{
		bZoomReady = false;
	}
	else if(bWantsToZoom)
	{
		ServerChangeZoom();
		bZoomLocation = false;
	}
}

void AMainCharacter::QuitUser()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC != nullptr)
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit);
	}
}

void AMainCharacter::ServerChangeSound_Implementation(bool OnOff)
{
	if (!bDied)
	{
		FireSound(OnOff);
	}
	else
	{
		CharacterAudioComp->Stop();
	}
}

bool AMainCharacter::ServerChangeSound_Validate(bool OnOff)
{
	return true;
}

void AMainCharacter::ServerChangeZoom_Implementation()
{
	bWantsToZoom = !bWantsToZoom;
}

bool AMainCharacter::ServerChangeZoom_Validate()
{
	return true;
}

void AMainCharacter::OnHealthChanged(UMaingameHealthComponent* OwningHealthComp, float Health, float HealthDelta, 
	const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f && !bDied)
	{	
		//ServerCode, ClientCode => OnRep_bDied;
		bDied = true;

		GetMovementComponent()->StopMovementImmediately(); //무브컴포넌트 정지
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore );
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECR_Ignore);
		bAppearOrDisAppear = true;

		if (bBot)
		{
			StopFire();
		}
		else
		{
			StopFire();
			CharacterAudioComp3->SetIntParameter(FName("CharacterSound"), 0);
			CharacterAudioComp3->Play();
			AMainGamePlayerState* TempPlayerState = Cast<AMainGamePlayerState>(PlayerState);
			TempPlayerState->AddDeadScore();
		}
		CurrentWeapon->SetLifeSpan(3.0f);

		ReSpawnRequest(); //BP에서 구현 [ GameState에 구현된 함수 실행, 서버에 리스폰 요청 ]

		DetachFromControllerPendingDestroy();

		SetLifeSpan(6.0f); //6초 후에 폰 전체가 파괴
	}
}

void AMainCharacter::OnRep_bDied()
{
	//ClientCode

	GetMovementComponent()->StopMovementImmediately();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECR_Ignore);
	SetActorTickEnabled(true);
	bAppearOrDisAppear = true;

	StopFire();

	CurrentWeapon->SetLifeSpan(5.0f);
}

void AMainCharacter::FallDeath()
{
	if (Role == ROLE_Authority)
	{
		bDied = true;

		GetMovementComponent()->StopMovementImmediately(); //무브컴포넌트 정지
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECR_Ignore);
		SetActorTickEnabled(true);

		if (bBot)
		{
			StopFire();
		}
		else
		{
			StopFire();
			AMainGamePlayerState* TempPlayerState = Cast<AMainGamePlayerState>(PlayerState);
			TempPlayerState->AddDeadScore();
		}

		CurrentWeapon->SetLifeSpan(3.0f);

		ReSpawnRequest();

		DetachFromControllerPendingDestroy();
	
		SetLifeSpan(6.0f); //10초 후에 폰 전체가 파괴
	}
}

void AMainCharacter::ClearDeath(float DisappearTime)
{
	if (Role == ROLE_Authority)
	{
		bDied = true;

		GetMovementComponent()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECR_Ignore);
		SetActorTickEnabled(true);

		StopFire();

		CurrentWeapon->SetLifeSpan(1.0f);
		DetachFromControllerPendingDestroy();
		SetLifeSpan(DisappearTime);
	}
}

void AMainCharacter::OnRep_bFire()
{
	if (!bDied)
	{
		if (bFire)
		{
			CharacterAudioComp->SetIntParameter(FName("RifleSound"), 0);
			CharacterAudioComp->Play();
		}
		else
		{
			CharacterAudioComp->SetIntParameter(FName("RifleSound"), 1);
			CharacterAudioComp->Play();
		}
	}
	else
	{
		CharacterAudioComp->Stop();
	}
}

void AMainCharacter::StartFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();

		if (!bDied)
		{
			if (Role < ROLE_Authority)
			{
				ServerChangeSound(true);
			}
			else
			{
				bFire = true;
				FireAddReportNoiseEvent();
			}
		}
	}
	if (bDied)
	{
		CharacterAudioComp->Stop();
	}
}

void AMainCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();

		if (!bDied)
		{
			if (Role < ROLE_Authority)
			{
				ServerChangeSound(false);
			}
			else
			{
				bFire = false;
				CharacterAudioComp->SetIntParameter(FName("RifleSound"), 1);
				CharacterAudioComp->Play();
			}
		}
	}
	if (bDied)
	{
		CharacterAudioComp->Stop();
	}
}

void AMainCharacter::StartBotFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartBotFire();

		if (!bDied)
		{
			if (Role < ROLE_Authority)
			{
				ServerChangeSound(true);
			}
			else
			{
				bFire = true;
				CharacterAudioComp->SetIntParameter(FName("RifleSound"), 0);
				CharacterAudioComp->Play();
			}
		}
	}
	if (bDied)
	{
		CharacterAudioComp->Stop();
	}
}

void AMainCharacter::FireSound(bool OnOff)
{
	if (!bDied)
	{
		bFire = OnOff;

		if (OnOff)
		{
			if (bBot)
			{
				CharacterAudioComp->SetIntParameter(FName("RifleSound"), 0);
				CharacterAudioComp->Play();
			}
			else
			{
				FireAddReportNoiseEvent();
			}
		}
		else
		{
			CharacterAudioComp->SetIntParameter(FName("RifleSound"), 1);
			CharacterAudioComp->Play();
		}
	}
	else
	{
		CharacterAudioComp->Stop();
	}
}

void AMainCharacter::StopFireSound()
{
	if (!bDied)
	{
		bFire = false;
		CharacterAudioComp->SetIntParameter(FName("RifleSound"), 1);
		CharacterAudioComp->Play();
	}
	else
	{
		CharacterAudioComp->Stop();
	}
}

float AMainCharacter::ReturnPitch()
{
	return pitch2;
}

// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!bWasJumping && bZoomReady)
	{
		ServerChangeZoom();
		bWantsToZoom = true;
		bZoomReady = false;
	}
	float TargetFOV = bWantsToZoom ? ZooomedFOV : DefalutFOV;

	float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, ZoomInterSpeed);
	//ZoomInterSpeed == FOV Change 감속

	CameraComp->SetFieldOfView(NewFOV);
}

// Called to bind functionality to input
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMainCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMainCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &AMainCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &AMainCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AMainCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AMainCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &AMainCharacter::BeginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &AMainCharacter::EndZoom);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AMainCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AMainCharacter::StopFire);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMainCharacter::Jump);

	PlayerInputComponent->BindAction("QuitGame", IE_Pressed, this, &AMainCharacter::QuitUser);
}

FVector AMainCharacter::GetPawnViewLocation() const
{
	if (CameraComp)
	{
		return CameraComp->GetComponentLocation();
	}
	 
	return Super::GetPawnViewLocation();
}

void AMainCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //무엇을 복제하고 어떤 복제방법을 선택할지 정하는 함수?
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMainCharacter, bDied);
	DOREPLIFETIME(AMainCharacter, CurrentWeapon);
	DOREPLIFETIME(AMainCharacter, bWantsToZoom);
	DOREPLIFETIME(AMainCharacter, bFire);
}