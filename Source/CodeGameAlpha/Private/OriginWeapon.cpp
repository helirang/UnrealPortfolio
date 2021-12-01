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
	ECVF_Cheat); //�ֺܼ���

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

	//��Ƽ_��Ʈ��ũ ������Ʈ ��
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
	//FMath::Max == (1,2) 1 �Ǵ� 2�߿��� ū ���� ����. �� 1�� �����̸� 2�� 0.0f�� ��ȯ�ȴ�.

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &AOriginWeapon::Fire, TimeBetweenShots, true, FirstDelay);
	// �߻� ���� ����, ����� 0.1�ʸ��� ȣ��
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
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation); //Owner�� ActorEyes������ ���
		FVector EyeDirection = EyeRotation.Vector();
		AimLocation = EyeLocation + (EyeDirection * 10000);
		MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FVector ShotDirection = GetDirection(MuzzleLocation, AimLocation);

		////Bullet Spread
		float HalfRad = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);

		FVector TraceEnd = MuzzleLocation + (ShotDirection * 10000); //��� == ��Ÿ�

		FCollisionQueryParams QueryParams; // �浹����, ����ü
		QueryParams.AddIgnoredActor(MyOwner);//�߰��� ���� Ignored
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true; //�����ϰ� �ִ� �޽��� �� ���� �ﰢ���� ���� ����. Trace�� false�̸� �ܼ� ������ ����
		QueryParams.bReturnPhysicalMaterial = true;
		
		//Particle "Target" parameter
		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit; 

		if (GetWorld()->LineTraceSingleByChannel(Hit, MuzzleLocation, TraceEnd, ECC_GameTraceChannel1, QueryParams))
		{
			//Hit ����ü, EyeLocation(��� ��ġ), TraceEnd(���� ��ġ), ä��( ��ħ, ���� ��� üũ�ϴ� ä��), �浹����

			AActor* HitActor = Hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;
			if (SurfaceType == SURFACE_FLESHVULNERABLE)
			{
				ActualDamage *= 4.0f;
			}

			//Fire ������ ��Ȱ��ȭ�ϸ� ���� �������� �ݿ� �ȵ�
			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection,Hit, MyOwner->GetInstigatorController()
				, MyOwner, DamageType);
			//HitActor, 20������, �߻����, Hit����ü, �߻��� ����, DamageType
			//TSubclassOf<UDamageType>�� ȭ�� �������� �� ������ ���� ����
			UMaingameHealthComponent* CheckFriend = 
				Cast<UMaingameHealthComponent>(MyOwner->GetComponentByClass(UMaingameHealthComponent::StaticClass()));

			//@TODO �����غ� �� == IsFriendly�κ� �ѱ����� �����غ���, SurfaceType ������� ���ƺ���
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

		FVector TraceEnd = StartPosition + (ShotDirection * 10000); //��� == ��Ÿ�

		FCollisionQueryParams QueryParams; // �浹����, ����ü
		QueryParams.AddIgnoredActor(MyOwner);//�߰��� ���� Ignored
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true; //�����ϰ� �ִ� �޽��� �� ���� �ﰢ���� ���� ����. Trace�� false�̸� �ܼ� ������ ����
		QueryParams.bReturnPhysicalMaterial = true;

		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit;

		if (GetWorld()->LineTraceSingleByChannel(Hit, StartPosition, TraceEnd, ECC_GameTraceChannel1, QueryParams))
		{
			//Hit ����ü, EyeLocation(��� ��ġ), TraceEnd(���� ��ġ), ä��( ��ħ, ���� ��� üũ�ϴ� ä��), �浹����

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

	if (Role == ROLE_Authority) //���������� �۵�
	{
		if (TargetActor == nullptr)
		{
			return;
		}

		FRotator fireRotation = UKismetMathLibrary::FindLookAtRotation(MyOwner->GetActorLocation(), TargetActor->GetActorLocation());
		MuzzleLocation = GetActorLocation() - (MyOwner->GetActorForwardVector() * 50);
		FVector fireDirection = UKismetMathLibrary::GetForwardVector(fireRotation);

		// ȸ����Ų Vector�� Z���� ��� == ĳ���� ����(x,y) ����(z)���� ����
		FVector ShotDirection = FVector(MyOwner->GetActorForwardVector().X, MyOwner->GetActorForwardVector().Y, fireDirection.Z);

		// ź ����
		float HalfRad = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);

		// ź ������ ����� �߻� ������
		FVector TraceEnd = MuzzleLocation + ((ShotDirection * 10000));

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit;

		// ��弦 ���� ����
		if (GetWorld()->LineTraceSingleByChannel(Hit, MuzzleLocation, TraceEnd, ECC_GameTraceChannel1, QueryParams))
		{
			AActor* HitActor = Hit.GetActor();

			if (DebugWeaponDrawing > 0)
			{
				DrawDebugLine(GetWorld(), MuzzleLocation, TraceEnd, FColor::Red, false, 3.0f, 0, 3.0f);
			}

			// ��ȯ�Ǵ� SurfaceType�� ���� ��ƼŬ ���� �� ������ ó��
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

			// Health�� TeamNum�� ������ �Ʊ����� ����, �Ʊ��̸� SurfaceType ����
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
			//��ƼŬ ����, ������ġ MuzzleLocation

			if (TracerComp)
			{
				TracerComp->SetVectorParameter(TracerTargetName, TracerEnd);
				//TracerComp�� "Target" ���� �Ķ���Ϳ� TracerEndPoint( ��ƼŬ �ý����� ������ )�� Set�Ѵ�.
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

	//SurfaceType�� Ȱ���� Effect ����
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
		//ImpactPoint ������, ShotDirection.Rotation() ����
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
	//DOREPLIFETIME ��ũ�� == ������ ����� Ŭ���̾�Ʈ�� Replicate
	//COND_SkipOwner == SkipOwner
}

/*Ŭ���̾�Ʈ->ServerFire->�������� Fire ����->�������� Clientĳ���Ͱ� Fire()�ϸ鼭 HitScanTrace�� Update->ReplicatedUsing�������� ���� �� Ŭ����Ʈ�� �ִ�
�ٸ� Client���� OnRep_HitScanTrace�� �۵� -> Ŭ���̾�Ʈ���� �ٸ� ������ ĳ���͵��� �߻��ϴ� ������� ǥ����*/