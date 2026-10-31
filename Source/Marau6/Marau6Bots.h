#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Marau6Bots.generated.h"
class AMarau6Bot;
/** Arena population is server-owned and keyed to the confirmed persistent raid. */
UCLASS()
class UMarau6Bots : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Tick(float Delta) override;
    virtual TStatId GetStatId()const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMarau6Bots,STATGROUP_Tickables); }
    void ResetForRound(const FString& Round);
    int32 GetSpawnedCount() const { return Spawned.Num(); }
private:
    FString CurrentRound;
    TSet<FName> Spawned;
    float NextCheck=0;
    virtual bool DoesSupportWorldType(EWorldType::Type T)const override{return T==EWorldType::Game || T==EWorldType::PIE;}
};
