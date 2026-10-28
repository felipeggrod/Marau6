// Copyright Epic Games, Inc. All Rights Reserved.

#include "Marau6Character.h"
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
#include "DrawDebugHelpers.h"


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
	if (IsAlive() && GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AMarau6Character::DoJumpStart()
{
	if (!IsAlive()) return;
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
    if (!HasAuthority() || !CanBeDamaged() || !IsAlive()
        || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f)
    {
        return 0.0f;
    }

    const float AppliedDamage = FMath::Min(DamageAmount, Health);
    Health = FMath::Clamp(Health - AppliedDamage, 0.0f, 100.0f);

    // RepNotify runs on receiving clients, so also update the server explicitly.
    OnRep_Health();
    ForceNetUpdate();
    Super::TakeDamage(AppliedDamage, DamageEvent, EventInstigator, DamageCauser);
    return AppliedDamage;
}


void AMarau6Character::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
    if (IsLocallyControlled() && IsAlive())
    {
        ServerFire();
    }
}

void AMarau6Character::ServerFire_Implementation()
{
    if (!HasAuthority() || !IsAlive() || !GetController() || !GetWorld())
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
    const FVector End = Start + Aim.Vector() * PrototypeRange;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(Marau6PrototypeFire), false, this);
    FHitResult Hit;
    // Pawn capsules and solid world surfaces block this channel. The capsule
    // intentionally gives uniform whole-body damage in this first prototype.
    const bool bBlockingHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Query);
    AMarau6Character* Target = bBlockingHit ? Cast<AMarau6Character>(Hit.GetActor()) : nullptr;
    const bool bHitCharacter = Target && Target->IsAlive();
    if (bHitCharacter)
    {
        FDamageEvent DamageEvent;
        Target->TakeDamage(PrototypeDamage, DamageEvent, GetController(), this);
    }
    MulticastShot(Start, bBlockingHit ? Hit.ImpactPoint : End, bHitCharacter);
}

void AMarau6Character::MulticastShot_Implementation(
    FVector_NetQuantize Start, FVector_NetQuantize End, bool bHitCharacter)
{
    if (GetNetMode() != NM_DedicatedServer)
    {
        DrawDebugLine(GetWorld(), Start, End,
            bHitCharacter ? FColor::Green : FColor::Red, false, 0.2f, 0, 1.5f);
        DrawDebugPoint(GetWorld(), End, 8.0f,
            bHitCharacter ? FColor::Green : FColor::Red, false, 0.3f);
    }
}
