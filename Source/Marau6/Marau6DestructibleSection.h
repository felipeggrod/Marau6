#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Marau6DestructibleSection.generated.h"
class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UNavModifierComponent;
class UMaterialInstanceDynamic;
/** One server-owned section; a destroyed section stays replicated for late clients. */
UCLASS()
class AMarau6DestructibleSection : public AActor
{
    GENERATED_BODY()
public:
    AMarau6DestructibleSection();
    float GetHealth() const{return Health;}
    bool IsBroken() const{return Health<=0;}
    int32 GetBarrierIndex() const{return BarrierIndex;}
    int32 GetSectionIndex() const{return SectionIndex;}
    void SetArenaIdentity(int32 Barrier,int32 Section);
    void ResetSection();
    virtual float TakeDamage(float Damage,const FDamageEvent& Event,AController* Instigator,AActor* Causer) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Props) const override;
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY(ReplicatedUsing=OnRep_Health) float Health=200.f;
    UPROPERTY(Replicated) uint32 Generation=0;
    UPROPERTY(Replicated) int32 BarrierIndex=-1;
    UPROPERTY(Replicated) int32 SectionIndex=-1;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Bounds;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UNavModifierComponent> NavigationBlock;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Stripes;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Cracks;
    UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> Labels;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BlueMaterial;
    UFUNCTION() void OnRep_Health();
    UFUNCTION(NetMulticast,Unreliable) void MulticastBreak(uint32 BrokenGeneration);
};

/** Visual only: never replicated, never collides, never contributes navigation. */
UCLASS(NotPlaceable,Transient)
class AMarau6SectionDebris : public AActor
{
    GENERATED_BODY()
public:
    AMarau6SectionDebris();
    virtual void Tick(float Delta) override;
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Pieces;
    float Age=0;
};
