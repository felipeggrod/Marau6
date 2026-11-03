#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Marau6InventoryTypes.h"
#include "Dom/JsonObject.h"
#include "Marau6ItemLedger.generated.h"
class UMarau6InventoryComponent;
class AMarau6LootActor;
enum class EMarau6Custody : uint8 { Inventory, World, Result, Recovery, Transfer, Consumed };
struct FMarau6ItemRecord
{
    FMarau6Item Item;
    EMarau6Custody Custody=EMarau6Custody::Inventory;
    FGuid OwnerId;
    TWeakObjectPtr<AMarau6LootActor> Loot;
    FVector RecoveryLocation=FVector::ZeroVector;
};
/** The single authoritative item store for this game-world session. */
UCLASS()
class UMarau6ItemLedger : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    bool AbandonReserved(FGuid Owner,const TArray<FMarau6Item>& Items,FVector Location=FVector(1400,350,90));
    bool FinalizeDisconnected(FGuid Owner,FVector Location);
    bool Import(UMarau6InventoryComponent* Component,FGuid Owner,const TArray<FMarau6Item>& Items);
    TArray<TSharedPtr<FJsonValue>> Export() const;
    bool IsFinalized(FGuid Owner) const { return Finalized.Contains(Owner); }
    bool IsExtracted(FGuid Owner) const { return ExtractedOwners.Contains(Owner); }
    void ResetRound();
    bool GrantWorld(FGuid Id,EMarau6ItemKind Kind,FVector Location);
    /** Raid entry with an empty weapon slot: a fresh rifle, born as loot (backend rule) then equipped. */
    bool GrantStarterRifle(UMarau6InventoryComponent* Component);
    FGuid RegisterInventory(UMarau6InventoryComponent* Component);
    void RegisterSeed(AMarau6LootActor* Actor, EMarau6ItemKind Kind);
    bool Pickup(UMarau6InventoryComponent* Component, AMarau6LootActor* Actor, FGuid Expected);
    bool Move(UMarau6InventoryComponent* Component, FGuid Expected, int32 Destination);
    bool Drop(UMarau6InventoryComponent* Component, FGuid Expected);
    bool ConsumeForHealing(UMarau6InventoryComponent* Component,FGuid Id);
    bool SaveShot(UMarau6InventoryComponent* Component);
    bool SaveAmmo(UMarau6InventoryComponent* Component, int32 Magazine, int32 Reserve);
    bool Finalize(UMarau6InventoryComponent* Component, bool bExtracted);
    void LootRemoved(AMarau6LootActor* Actor, FGuid Id);
    TArray<FMarau6Item> GetHeldResult(FGuid OwnerId) const;
    int32 GetItemCount() const { return Records.Num(); }
    int32 GetRecoveryCount() const;
    bool IsInventoryBusy(FGuid Owner) const { return BusyOwners.Contains(Owner); }
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMarau6ItemLedger,STATGROUP_Tickables); }
private:
    friend class UMarau6InventoryComponent;
    bool Persist();
    bool IsServer() const;
    bool Usable(UMarau6InventoryComponent* Component) const;
    void Sync(FGuid OwnerId);
    FMarau6ItemRecord* Owned(UMarau6InventoryComponent* Component, FGuid Id);
    FMarau6ItemRecord* At(FGuid OwnerId,int32 Slot);
    int32 Empty(FGuid OwnerId,int32 Start,int32 Count);
    bool SpawnWorld(FGuid Id, FVector Location);
    TMap<FGuid,FMarau6ItemRecord> Records;
    TMap<FGuid,TWeakObjectPtr<UMarau6InventoryComponent>> Owners;
    TSet<FGuid> BusyOwners;
    TSet<FGuid> Finalized;
    TSet<FGuid> ExtractedOwners;
    float RecoverySeconds=0.f;
    TMap<FGuid,int32> LostCounts;
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
};
