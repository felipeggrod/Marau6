#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Marau6Destruction.generated.h"
class AMarau6DestructibleSection;
/** Temporary round scenery, never part of the durable item ledger. */
UCLASS()
class UMarau6Destruction : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void OnWorldBeginPlay(UWorld& World) override;
    void ResetForRound(const FString& Round);
    int32 GetSectionCount() const;
    bool IsReadyForRound() const{return bNavigationReady;}
    virtual void Tick(float Delta) override;
    virtual TStatId GetStatId() const override {RETURN_QUICK_DECLARE_CYCLE_STAT(UMarau6Destruction,STATGROUP_Tickables);}
private:
    void EnsureArena();
    FString CurrentRound;
    UPROPERTY() bool bNavigationReady=false;
    uint64 ResetFrame=0;
    TArray<TWeakObjectPtr<AMarau6DestructibleSection>> Sections;
    virtual bool DoesSupportWorldType(EWorldType::Type T) const override{return T==EWorldType::Game || T==EWorldType::PIE;}
};
