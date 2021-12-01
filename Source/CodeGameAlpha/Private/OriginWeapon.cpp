// Fill out your copyright notice in the Description page of Project Settings.

#include "OriginWeapon.h"
#include "CodeGameAlpha.h"
#include "Components//SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "MaingameHealthComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials//PhysicalMaterial.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(
	TEXT("MainGame.DebugWeapons"),
	DebugWeaponDrawing,
	TEXT("Draw Debug Lines for Weapons"),
	ECVF_Cheat); //콘솔변수

// Sets default values
AOriginWeapon::AOriginWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	ButtstockSocketName = "ButtstockSocket";
	MuzzleSocketName = "MuzzleSocket";
	MuzzleEffectSocketName = "MuzzleEffectSocket";
	TracerTargetName = "Target";
	BaseDamage = 20.0f;
	BulletSpread = 2.0f;
	RateOfFire = 600;

	SetReplicates(true);

	//멀티_네트워크 업데이트 빈도
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	bIsBot = false;
}

// Called when the game starts or when spawned
void AOriginWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	TimeBetweenShots = 60 / RateOfFire;
}

void AOriginWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);
	//FMath::Max == (1,2) 1 또는 2중에서 큰 놈을 고른다. 즉 1이 음수이면 2인 0.0f가 반환된다.

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &AOriginWeapon::Fire, TimeBetweenShots, true, FirstDelay);
	// 발사 간격 조정, 현재는 0.1초마다 호출
}

void AOriginWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}


void AOriginWeapon::StartBotFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);
	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &AOriginWeapon::BotFire, TimeBetweenShots, true, FirstDelay);
}

void AOriginWeapon::Fire()
{
	AActor* MyOwner = GetOwner();

	if (MyOwner)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation); //Owner의 ActorEyes관점을 사용
		FVector EyeDirection = EyeRotation.Vector();
		AimLocation = EyeLocation + (EyeDirection * 10000);
		MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FVector ShotDirection = GetDirection(MuzzleLocation, AimLocation);

		////Bullet Spread
		float HalfRad = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);

		FVector TraceEnd = MuzzleLocation + (ShotDirection * 10000); //상수 == 사거리

		FCollisionQueryParams QueryParams; // 충돌쿼리, 구조체
		QueryParams.AddIgnoredActor(MyOwner);//추가된 액터 Ignored
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true; //공격하고 있는 메시의 각 개별 삼각형에 대해 추적. Trace가 false이면 단순 수집만 수행
		QueryParams.bReturnPhysicalMaterial = true;
		
		//Particle "Target" parameter
		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit; 

		if (GetWorld()->LineTraceSingleByChannel(Hit, MuzzleLocation, TraceEnd, ECC_GameTraceChannel1, QueryParams))
		{
			//Hit 구조체, EyeLocation(출발 위치), TraceEnd(도착 위치), 채널( 겹침, 차단 등등 체크하는 채널), 충돌쿼리

			AActor* HitActor = Hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;
			if (SurfaceType == SURFACE_FLESHVULNERABLE)
			{
				ActualDamage *= 4.0f;
			}

			//Fire 데미지 비활성화하면 서버 데미지가 반영 안됨
			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection,Hit, MyOwner->GetInstigatorController()
				, MyOwner, DamageType);
			//HitActor, 20데미지, 발사방향, Hit구조체, 발사자 정보, DamageType
			//TSubclassOf<UDamageType>로 화염 데미지나 독 데미지 구현 가능
			UMaingameHealthComponent* CheckFriend = 
				Cast<UMaingameHealthComponent>(MyOwner->GetComponentByClass(UMaingameHealthComponent::StaticClass()));

			//@TODO 개량해볼 것 == IsFriendly부분 한군데로 통일해보기, SurfaceType 사용하지 말아보기
			if ( CheckFriend != nullptr&& CheckFriend->IsFriendly(MyOwner,HitActor))
			{
				if (MyOwner == nullptr || HitActor == nullptr)
				{
				}	
				else
				{
					UMaingameHealthComponent* HealthCompA = 
						Cast<UMaingameHealthComponent>(MyOwner->GetComponentByClass(UMaingameHealthComponent::StaticClass()));
					UMaingameHealthComponent* HealthCompB = 
						Cast<UMaingameHealthComponent>(HitActor->GetComponentByClass(UMaingameHealthComponent::StaticClass()));
					
					if (HealthCompA == nullptr || HealthCompB == nullptr)
					{
					}
					else
					{
						SurfaceType = SURFACE_FLESHCANCELLED;
					}
				}
			}
			PlayImpactEffect(SurfaceType, Hit.ImpactPoint);
			TracerEndPoint = Hit.ImpactPoint;
		}

		if (DebugWeaponDrawing > 0)
		{
			DrawDebugLine(GetWorld(), MuzzleLocation, TraceEnd, FColor::Yellow, false, 3.0f, 0, 3.0f);
		}

		PlayFireEffects(TracerEndPoint);

		if (Role == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.Surfacetype = SurfaceType;
		}

		LastFireTime = GetWorld()->TimeSeconds;
	}

	if (Role < ROLE_Authority) //Server Check
	{
		ServerFire(MuzzleLocation, AimLocation); 
	}
}

void AOriginWeapon::Fire(FVector StartPosition, FVector EndPosition)
{

	AActor* MyOwner = GetOwner();

	if (MyOwner)
	{
		FVector ShotDirection = GetDirection(StartPosition, EndPosition);

		////Bullet Spread
		float HalfRad = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);

		FVector TraceEnd = StartPosition + (ShotDirection * 10000); //상수 == 사거리

		FCollisionQueryParams QueryParams; // 충돌쿼리, 구조체
		QueryParams.AddIgnoredActor(MyOwner);//추가된 액터 Ignored
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true; //공격하고 있는 메시의 각 개별 삼각형에 대해 추적. Trace가 false이면 단순 수집만 수행
		QueryParams.bReturnPhysicalMaterial = true;

		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit;

		if (GetWorld()->LineTraceSingleByChannel(Hit, StartPosition, TraceEnd, ECC_GameTraceChannel1, QueryParams))
		{
			//Hit 구조체, EyeLocation(출발 위치), TraceEnd(도착 위치), 채널( 겹침, 차단 등등 체크하는 채널), 충돌쿼리

			AActor* HitActor = Hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;
			if (SurfaceType == SURFACE_FLESHVULNERABLE)
			{
				ActualDamage *= 4.0f;
			}

			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController()
				, MyOwner, DamageType);
			UMaingameHealthComponent* CheckFriend = 
				Cast<UMaingameHealthComponent>(MyOwner->GetComponentByClass(UMaingameHealthComponent::StaticClass()));

			if (CheckFriend != nullptr&& CheckFriend->IsFriendly(MyOwner, HitActor))
			{
				if (MyOwner == nullptr || HitActor == nullptr)
				{
				}
				else
				{
					UMaingameHealthComponent* HealthCompA = 
						Cast<UMaingameHealthComponent>(MyOwner->GetComponentByClass(UMaingameHealthComponent::StaticClass()));
					UMaingameHealthComponent* HealthCompB = 
						Cast<UMaingameHealthComponent>(HitActor->GetComponentByClass(UMaingameHealthComponent::StaticClass()));

					if (HealthCompA == nullptr || HealthCompB == nullptr)
					{
					}
					else
					{
						SurfaceType = SURFACE_FLESHCANCELLED;
					}
				}
			}
			PlayImpactEffect(SurfaceType, Hit.ImpactPoint);
			TracerEndPoint = Hit.ImpactPoint;
		}

		if (DebugWeaponDrawing > 0)
		{
			DrawDebugLine(GetWorld(), StartPosition, TraceEnd, FColor::Yellow, false, 3.0f, 0, 3.0f);
		}

		PlayFireEffects(TracerEndPoint);

		if (Role == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.Surfacetype = SurfaceType;
		}

		LastFireTime = GetWorld()->TimeSeconds;

	}
}

void AOriginWeapon::BotFire()
{
	AActor* MyOwner = GetOwner();

	if (Role == ROLE_Authority) //서버에서만 작동
	{
		if (TargetActor == nullptr)
		{
			return;
		}

		FRotator fireRotation = UKismetMathLibrary::FindLookAtRotation(MyOwner->GetActorLocation(), TargetActor->GetActorLocation());
		MuzzleLocation = GetActorLocation() - (MyOwner->GetActorForwardVector() * 50);
		FVector fireDirection = UKismetMathLibrary::GetForwardVector(fireRotation);

		// 회전시킨 Vector의 Z값만 사용 == 캐릭터 정면(x,y) 상하(z)값만 변경
		FVector ShotDirection = FVector(MyOwner->GetActorForwardVector().X, MyOwner->GetActorForwardVector().Y, fireDirection.Z);

		// 탄 퍼짐
		float HalfRad = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);

		// 탄 퍼짐이 적용된 발사 도착지
		FVector TraceEnd = MuzzleLocation + ((ShotDirection * 10000));

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit;

		// 헤드샷 등의 판정
		if (GetWorld()->LineTraceSingleByChannel(Hit, MuzzleLocation, TraceEnd, ECC_GameTraceChannel1, QueryParams))
		{
			AActor* HitActor = Hit.GetActor();

			if (DebugWeaponDrawing > 0)
			{
				DrawDebugLine(GetWorld(), MuzzleLocation, TraceEnd, FColor::Red, false, 3.0f, 0, 3.0f);
			}

			// 반환되는 SurfaceType에 따라 파티클 생성 및 데미지 처리
			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;

			if (SurfaceType == SURFACE_FLESHVULNERABLE)
			{
				ActualDamage *= 4.0f;
			}

			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController()
				, MyOwner, DamageType);
			UMaingameHealthComponent* CheckFriend =
				Cast<UMaingameHealthComponent>(MyOwner->GetComponentByClass(UMaingameHealthComponent::StaticClass()));

			// Health에 TeamNum을 가지고 아군인지 판정, 아군이면 SurfaceType 변경
			if (CheckFriend != nullptr && CheckFriend->IsFriendly(MyOwner, HitActor))
			{
				if (MyOwner == nullptr || HitActor == nullptr)
				{
				}
				else
				{
					UMaingameHealthComponent* HealthCompA =
						Cast<UMaingameHealthComponent>(MyOwner->GetComponentByClass(UMaingameHealthComponent::StaticClass()));
					UMaingameHealthComponent* HealthCompB =
						Cast<UMaingameHealthComponent>(HitActor->GetComponentByClass(UMaingameHealthComponent::StaticClass()));

					if (HealthCompA == nullptr || HealthCompB == nullptr)
					{
					}
					else
					{
						SurfaceType = SURFACE_FLESHCANCELLED;
					}
				}
			}

			PlayImpactEffect(SurfaceType, Hit.ImpactPoint);
			TracerEndPoint = Hit.ImpactPoint;
		}

		if (DebugWeaponDrawing > 0)
		{
			DrawDebugLine(GetWorld(), MuzzleLocation, TraceEnd, FColor::Yellow, false, 3.0f, 0, 3.0f);
		}

		PlayFireEffects(TracerEndPoint);

		if (Role == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.Surfacetype = SurfaceType;
		}

		LastFireTime = GetWorld()->TimeSeconds;
	}
}

//Effect
void AOriginWeapon::PlayFireEffects(FVector TracerEnd)
{
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleEffectSocketName);
	}

	if (TracerEffect)
	{
		if (bIsBot)
		{
			AActor* MyOwner = GetOwner();

			MuzzleLocation = GetActorLocation() - (MyOwner->GetActorForwardVector() * 50);

			FVector ShotDirection = GetDirection(MuzzleLocation, TracerEnd);
			FVector TracerStartLocation = MuzzleLocation + (ShotDirection * 200);

			UParticleSystemComponent* TracerComp = 
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, TracerStartLocation);

			if (TracerComp)
			{
				TracerComp->SetVectorParameter(TracerTargetName, TracerEnd);
			}
		}
		else
		{
			FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
			FVector ButtstockLocation = MeshComp->GetSocketLocation(ButtstockSocketName);

			FVector ShotDirection = GetDirection(ButtstockLocation, MuzzleLocation);
			FVector TracerStartLocation = MuzzleLocation + (ShotDirection * 25);

			UParticleSystemComponent* TracerComp = 
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, TracerStartLocation);
			//파티클 생성, 생성위치 MuzzleLocation

			if (TracerComp)
			{
				TracerComp->SetVectorParameter(TracerTargetName, TracerEnd);
				//TracerComp의 "Target" 백터 파라미터에 TracerEndPoint( 파티클 시스템의 도착지 )를 Set한다.
			}
		}
	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}
}

void AOriginWeapon::PlayImpactEffect(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{

	UParticleSystem* SelectedEffect = nullptr;

	//SurfaceType을 활용한 Effect 설정
	switch (SurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
	case SURFACE_FLESHVULNERABLE:
		SelectedEffect = FleshImpactEffect;
		break;
	case SURFACE_FLESHCANCELLED:
		SelectedEffect = nullptr;
		break;
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}

	if (SelectedEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
		//ImpactPoint 도착지, ShotDirection.Rotation() 방향
		if (Role == ROLE_Authority)
		{
			ReportNoiseImpactPoint(ImpactPoint);
		}
	}
}

FVector AOriginWeapon::GetDirection(FVector StartLocation, FVector EndLocation)
{
	FVector MuzzleDirection;
	MuzzleDirection = EndLocation - StartLocation;
	MuzzleDirection.Normalize();
	return MuzzleDirection;
}

// Net
void AOriginWeapon::ServerFire_Implementation(FVector StartPosition, FVector EndPosition)
{
	Fire(StartPosition,EndPosition);
}

bool AOriginWeapon::ServerFire_Validate(FVector StartPosition, FVector EndPosition)
{
	return true;
}

void AOriginWeapon::OnRep_HitScanTrace()
{
	PlayFireEffects(HitScanTrace.TraceTo);

	PlayImpactEffect(HitScanTrace.Surfacetype, HitScanTrace.TraceTo);
}

void AOriginWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AOriginWeapon, HitScanTrace, COND_SkipOwner);
	DOREPLIFETIME(AOriginWeapon, bIsBot);
	//DOREPLIFETIME 매크로 == 서버와 연결된 클라이언트에 Replicate
	//COND_SkipOwner == SkipOwner
}

/*클라이언트->ServerFire->서버에서 Fire 실행->서버에서 Client캐릭터가 Fire()하면서 HitScanTrace가 Update->ReplicatedUsing선언으로 인해 각 클라인트에 있는
다른 Client들의 OnRep_HitScanTrace가 작동 -> 클라이언트에서 다른 유저의 캐릭터들이 발사하는 모습으로 표현됨*/