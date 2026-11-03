#include "Marau6ShotEffect.h"
#include "Marau6Audio.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AMarau6ShotEffect::AMarau6ShotEffect()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;
    static ConstructorHelpers::FObjectFinder<USoundBase> Sound(
        TEXT("/Game/Marau6/Audio/S_Shot_Uzi.S_Shot_Uzi"));
    ShotSound = Sound.Object;
    ShotAttenuation = CreateDefaultSubobject<USoundAttenuation>(TEXT("ShotAttenuation"));
    Marau6Audio::ConfigureGunfireFalloff(ShotAttenuation->Attenuation, Marau6Audio::GunfireInner, Marau6Audio::GunfireFalloff);
    Marau6Audio::ConfigureDistanceSend(ShotAttenuation->Attenuation, .3f, .8f);
    OwnerShotAttenuation = CreateDefaultSubobject<USoundAttenuation>(TEXT("OwnerShotAttenuation"));
    Marau6Audio::ConfigureOwnerSend(OwnerShotAttenuation->Attenuation, .3f);
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    MuzzleFlash = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MuzzleFlash"));
    Impact = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Impact"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterial> Glow(TEXT("/Game/Marau6/Effects/M_ShotGlow.M_ShotGlow"));
    for (UStaticMeshComponent* Mesh : { MuzzleFlash.Get(), Impact.Get() })
    {
        Mesh->SetupAttachment(GetRootComponent());
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCastShadow(false);
        Mesh->SetCanEverAffectNavigation(false);
        Mesh->SetStaticMesh(Sphere.Object);
        Mesh->SetMaterial(0, Glow.Object);
        Mesh->SetVisibility(false);
    }
}

void AMarau6ShotEffect::Present(USkeletalMeshComponent* Rifle, bool bFirstPerson,
    const FVector& Point, const FVector& Normal, bool bBlockingHit, bool bHitCharacter)
{
    if (!GetWorld() || GetNetMode() == NM_DedicatedServer) { Destroy(); return; }
    // Fire-and-forget audio outlives this short visual effect. Only the owning
    // listener gets unpanned playback; other clients hear the world-space rifle.
    // Both feed the ambient reverb, distant shots more.
    if (ShotSound && Rifle)
    {
        const float Pitch = FMath::FRandRange(.95f, 1.05f);
        if (bFirstPerson)
        {
            UGameplayStatics::PlaySoundAtLocation(this, ShotSound, Rifle->GetComponentLocation(),
                FRotator::ZeroRotator, Marau6Audio::ShotOwner, Pitch, 0.f, OwnerShotAttenuation);
        }
        else
        {
            UGameplayStatics::PlaySoundAtLocation(this, ShotSound,
                Rifle->GetSocketLocation(TEXT("Muzzle")), FRotator::ZeroRotator,
                Marau6Audio::ShotWorld, Pitch, 0.f, ShotAttenuation);
        }
    }
    if (Rifle && Rifle->DoesSocketExist(TEXT("Muzzle")))
    {
        MuzzleFlash->AttachToComponent(Rifle, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("Muzzle"));
        MuzzleFlash->SetRelativeLocation(FVector(4,0,0));
        MuzzleFlash->SetRelativeScale3D(FVector(.12f,.035f,.035f));
        MuzzleFlash->FirstPersonPrimitiveType = bFirstPerson
            ? EFirstPersonPrimitiveType::FirstPerson : EFirstPersonPrimitiveType::WorldSpaceRepresentation;
        MuzzleFlash->SetVisibility(true);
        GetWorldTimerManager().SetTimer(FlashTimer, this, &AMarau6ShotEffect::HideFlash, .065f, false);
    }
    if (bBlockingHit)
    {
        SetActorLocation(Point + Normal.GetSafeNormal() * 1.5f);
        Impact->SetRelativeScale3D(FVector(.045f));
        if (UMaterialInstanceDynamic* Material = Impact->CreateDynamicMaterialInstance(0))
        {
            Material->SetVectorParameterValue(TEXT("GlowColor"), bHitCharacter
                ? FLinearColor(9, .4f, .15f) : FLinearColor(12, 7, 1));
        }
        Impact->SetVisibility(true);
    }
    SetLifeSpan(.22f);
}

void AMarau6ShotEffect::HideFlash() { MuzzleFlash->SetVisibility(false); }
void AMarau6ShotEffect::EndPlay(const EEndPlayReason::Type Reason)
{
    if (GetWorld()) GetWorldTimerManager().ClearTimer(FlashTimer);
    Super::EndPlay(Reason);
}
