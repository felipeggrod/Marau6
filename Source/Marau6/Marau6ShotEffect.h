#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Marau6ShotEffect.generated.h"
class USkeletalMeshComponent;
class UStaticMeshComponent;
class USoundBase;
class USoundAttenuation;

/** Local presentation only. Never participates in hit detection or replication. */
UCLASS(NotBlueprintable)
class AMarau6ShotEffect : public AActor
{
    GENERATED_BODY()
public:
    AMarau6ShotEffect();
    void Present(USkeletalMeshComponent* Rifle, bool bFirstPerson,
        const FVector& Point, const FVector& Normal, bool bBlockingHit, bool bHitCharacter);
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> MuzzleFlash;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Impact;
    UPROPERTY() TObjectPtr<USoundBase> ShotSound;
    UPROPERTY() TObjectPtr<USoundAttenuation> ShotAttenuation;
    FTimerHandle FlashTimer;
    void HideFlash();
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};
