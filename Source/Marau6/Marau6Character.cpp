// Copyright Epic Games, Inc. All Rights Reserved.

#include "Marau6Character.h"
#include "Marau6ExtractionZone.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Marau6.h"
#include "Net/UnrealNetwork.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/PlayerController.h"
#include "Marau6ShotEffect.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"


AMarau6Character::AMarau6Character()
{
	bReplicates = true;
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, BaseEyeHeight), FRotator::ZeroRotator);
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

    FirstPersonRifle = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonRifle"));
    FirstPersonRifle->SetupAttachment(FirstPersonMesh, TEXT("HandGrip_R"));
    FirstPersonRifle->SetCollisionProfileName(TEXT("NoCollision"));
    FirstPersonRifle->SetGenerateOverlapEvents(false);
    FirstPersonRifle->SetOnlyOwnerSee(true);
    FirstPersonRifle->SetCastShadow(false);
    FirstPersonRifle->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;

    ThirdPersonRifle = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ThirdPersonRifle"));
    ThirdPersonRifle->SetupAttachment(GetMesh(), TEXT("HandGrip_R"));
    ThirdPersonRifle->SetCollisionProfileName(TEXT("NoCollision"));
    ThirdPersonRifle->SetGenerateOverlapEvents(false);
    ThirdPersonRifle->SetOwnerNoSee(true);
    ThirdPersonRifle->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Rifle(
        TEXT("/Game/Weapons/Rifle/Meshes/SKM_Rifle.SKM_Rifle"));
    if (Rifle.Succeeded())
    {
        FirstPersonRifle->SetSkeletalMesh(Rifle.Object);
        ThirdPersonRifle->SetSkeletalMesh(Rifle.Object);
    }

    // These graphs refer back to character Blueprints. Defer loading until
    // after native class defaults have finished constructing.
    PrototypeFirstPersonPose = FSoftObjectPath(
        TEXT("/Game/Marau6/Anims/ABP_Marau6_FP_Rifle.ABP_Marau6_FP_Rifle_C"));
    PrototypeThirdPersonPose = FSoftObjectPath(
        TEXT("/Game/Marau6/Anims/ABP_Marau6_TP_Rifle.ABP_Marau6_TP_Rifle_C"));

}

void AMarau6Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{

        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
            {
                if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                    ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
                {
                    PrototypeFireAction = NewObject<UInputAction>(this);
                    PrototypeFireAction->ValueType = EInputActionValueType::Boolean;
                    PrototypeFireMapping = NewObject<UInputMappingContext>(this);
                    PrototypeFireMapping->MapKey(PrototypeFireAction, EKeys::LeftMouseButton);
                    PrototypeReloadAction = NewObject<UInputAction>(this);
                    PrototypeReloadAction->ValueType = EInputActionValueType::Boolean;
                    PrototypeFireMapping->MapKey(PrototypeReloadAction, EKeys::R);
                    EnhancedInputComponent->BindAction(PrototypeReloadAction, ETriggerEvent::Started,
                        this, &AMarau6Character::DoReload);
                    Subsystem->AddMappingContext(PrototypeFireMapping, 1);
                    EnhancedInputComponent->BindAction(PrototypeFireAction, ETriggerEvent::Started,
                        this, &AMarau6Character::DoFire);
                }
            }
        }

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMarau6Character::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMarau6Character::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMarau6Character::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMarau6Character::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMarau6Character::LookInput);
	}
	else
	{
		UE_LOG(LogMarau6, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AMarau6Character::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AMarau6Character::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AMarau6Character::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMarau6Character::DoMove(float Right, float Forward)
{
	if (CanParticipate() && GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AMarau6Character::DoJumpStart()
{
	if (!CanParticipate()) return;
	// pass Jump to the character
	Jump();
}

void AMarau6Character::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void AMarau6Character::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMarau6Character, Health);
    DOREPLIFETIME(AMarau6Character, RaidStatus);
    DOREPLIFETIME_CONDITION(AMarau6Character, AmmoState, COND_OwnerOnly);
}

void AMarau6Character::OnRep_Health()
{
    if (!IsAlive())
    {
        StopJumping();
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
    }

	UE_LOG(LogMarau6, Log, TEXT("%s: vida atual = %.0f"),
		*GetName(), Health);
}


float AMarau6Character::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (!HasAuthority() || !CanBeDamaged() || !CanParticipate()
        || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f)
    {
        return 0.0f;
    }

    const float AppliedDamage = FMath::Min(DamageAmount, Health);
    Health = FMath::Clamp(Health - AppliedDamage, 0.0f, 100.0f);

    if (!IsAlive())
    {
        CancelReload();
        RaidStatus.State = EMarau6RaidState::Eliminated;
        RaidStatus.ExtractionEndServerTime = 0.0;
        ExtractionZone.Reset();
        OnRep_RaidStatus();
    }

    // RepNotify runs on receiving clients, so also update the server explicitly.
    OnRep_Health();
    ForceNetUpdate();
    Super::TakeDamage(AppliedDamage, DamageEvent, EventInstigator, DamageCauser);
    return AppliedDamage;
}


void AMarau6Character::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelReload();
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
            {
                if (PrototypeFireMapping)
                {
                    Subsystem->RemoveMappingContext(PrototypeFireMapping);
                }
            }
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AMarau6Character::DoFire()
{
    if (IsLocallyControlled() && CanParticipate() && !IsReloading() && GetMagazineAmmo() > 0)
    {
        ServerFire();
    }
}

void AMarau6Character::ServerFire_Implementation()
{
    if (!HasAuthority() || !CanParticipate() || !GetController() || !GetWorld()
        || AmmoState.bReloading || AmmoState.Magazine <= 0)
    {
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();
    if (LastServerFireTime >= 0.0 && Now - LastServerFireTime < PrototypeFireInterval)
    {
        return;
    }

    // Neither origin, target, nor damage is supplied by the firing client.
    const FVector Start = GetPawnViewLocation();
    const FRotator Aim = GetControlRotation();
    if (Start.ContainsNaN() || Aim.ContainsNaN())
    {
        return;
    }
    LastServerFireTime = Now;
    --AmmoState.Magazine;
    ForceNetUpdate();
    const FVector End = Start + Aim.Vector() * PrototypeRange;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(Marau6PrototypeFire), false, this);
    FHitResult Hit;
    // Pawn capsules and solid world surfaces block this channel. The capsule
    // intentionally gives uniform whole-body damage in this first prototype.
    const bool bBlockingHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Query);
    AMarau6Character* Target = bBlockingHit ? Cast<AMarau6Character>(Hit.GetActor()) : nullptr;
    const bool bHitCharacter = Target && Target->CanParticipate();
    if (bHitCharacter)
    {
        FDamageEvent DamageEvent;
        Target->TakeDamage(PrototypeDamage, DamageEvent, GetController(), this);
    }
    MulticastShot(bBlockingHit ? Hit.ImpactPoint : End,
        bBlockingHit ? Hit.ImpactNormal : FVector::ZeroVector, bBlockingHit, bHitCharacter);
}

void AMarau6Character::MulticastShot_Implementation(
    FVector_NetQuantize Point, FVector_NetQuantizeNormal Normal, bool bBlockingHit, bool bHitCharacter)
{
    if (!GetWorld() || GetNetMode() == NM_DedicatedServer) return;
    // Only an accepted server shot reaches this cosmetic multicast.
    USkeletalMeshComponent* Rifle = IsLocallyControlled() ? FirstPersonRifle : ThirdPersonRifle;
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (AMarau6ShotEffect* Effect = GetWorld()->SpawnActor<AMarau6ShotEffect>(
        AMarau6ShotEffect::StaticClass(), FVector(Point), FRotator::ZeroRotator, Spawn))
    {
        Effect->Present(Rifle, IsLocallyControlled(), Point, Normal, bBlockingHit, bHitCharacter);
    }
}

void AMarau6Character::DoReload()
{
    if (IsLocallyControlled() && CanParticipate())
    {
        ServerReload();
    }
}

void AMarau6Character::ServerReload_Implementation()
{
    if (!HasAuthority() || !CanParticipate() || !GetController() || !GetWorld()
        || AmmoState.bReloading || AmmoState.Magazine >= PrototypeMagazineCapacity
        || AmmoState.Reserve <= 0)
    {
        return;
    }

    AmmoState.bReloading = true;
    GetWorldTimerManager().SetTimer(ReloadTimer, this,
        &AMarau6Character::FinishReload, PrototypeReloadDuration, false);
    ForceNetUpdate();
}

void AMarau6Character::FinishReload()
{
    if (!HasAuthority() || !AmmoState.bReloading)
    {
        return;
    }
    if (!CanParticipate() || !GetController())
    {
        CancelReload();
        return;
    }

    const int32 Needed = FMath::Max(0, PrototypeMagazineCapacity - AmmoState.Magazine);
    const int32 Transferred = FMath::Min(Needed, AmmoState.Reserve);
    AmmoState.Magazine += Transferred;
    AmmoState.Reserve -= Transferred;
    AmmoState.bReloading = false;
    ForceNetUpdate();
}

void AMarau6Character::CancelReload()
{
    if (!HasAuthority())
    {
        return;
    }
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ReloadTimer);
    }
    AmmoState.bReloading = false;
}


void AMarau6Character::BeginPlay()
{
    Super::BeginPlay();

    // The template pose initializes its camera when the owning actor has begun play.
    if (UClass* ThirdPose = PrototypeThirdPersonPose.LoadSynchronous())
    {
        GetMesh()->SetAnimInstanceClass(ThirdPose);
    }
    if (UClass* FirstPose = PrototypeFirstPersonPose.LoadSynchronous())
    {
        FirstPersonMesh->SetAnimInstanceClass(FirstPose);
    }

    // The first person graph copies the world body pose even when it is hidden
    // from the owning camera. Always refresh bones so the hand socket stays current.
    GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    FirstPersonMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    FirstPersonMesh->AddTickPrerequisiteComponent(GetMesh());
}

void AMarau6Character::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (HasAuthority()) UpdateExtraction();
}

float AMarau6Character::GetExtractionSecondsRemaining() const
{
    if (RaidStatus.State != EMarau6RaidState::Extracting || !GetWorld()) return 0.f;
    const AGameStateBase* State = GetWorld()->GetGameState();
    const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    return FMath::Max(0.f, static_cast<float>(RaidStatus.ExtractionEndServerTime - Now));
}

void AMarau6Character::UpdateExtraction()
{
    if (!HasAuthority() || !GetWorld() || !CanParticipate()) return;
    AMarau6ExtractionZone* Zone = ExtractionZone.Get();
    if (!GetController()) Zone = nullptr;
    else if (!IsValid(Zone) || !Zone->ContainsCharacter(this))
    {
        Zone = nullptr;
        for (TActorIterator<AMarau6ExtractionZone> It(GetWorld()); It; ++It)
        {
            if (It->ContainsCharacter(this)) { Zone = *It; break; }
        }
    }
    if (!Zone)
    {
        if (RaidStatus.State == EMarau6RaidState::Extracting)
        {
            RaidStatus.State = EMarau6RaidState::Active;
            RaidStatus.ExtractionEndServerTime = 0.0;
            ExtractionZone.Reset();
            ForceNetUpdate();
        }
        return;
    }
    const double Now = GetWorld()->GetTimeSeconds();
    if (RaidStatus.State != EMarau6RaidState::Extracting || Zone != ExtractionZone.Get())
    {
        ExtractionZone = Zone;
        RaidStatus.State = EMarau6RaidState::Extracting;
        RaidStatus.ExtractionEndServerTime = Now + FMath::Clamp(Zone->GetHoldSeconds(),1.f,60.f);
        ForceNetUpdate();
    }
    if (Now >= RaidStatus.ExtractionEndServerTime) CompleteExtraction();
}

void AMarau6Character::CompleteExtraction()
{
    if (!HasAuthority() || !CanParticipate() || RaidStatus.State != EMarau6RaidState::Extracting
        || !ExtractionZone.IsValid() || !ExtractionZone->ContainsCharacter(this)
        || !GetController() || GetWorld()->GetTimeSeconds() < RaidStatus.ExtractionEndServerTime) return;
    RaidStatus.State = EMarau6RaidState::Extracted;
    RaidStatus.ExtractionEndServerTime = 0.0;
    ExtractionZone.Reset();
    CancelReload();
    OnRep_RaidStatus();
    ForceNetUpdate();
}

void AMarau6Character::OnRep_RaidStatus()
{
    if (!CanParticipate())
    {
        StopJumping();
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
    }
    if (RaidStatus.State == EMarau6RaidState::Extracted)
    {
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
    }
}
