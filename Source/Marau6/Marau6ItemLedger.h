#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Marau6InventoryTypes.h"
#include "Marau6ItemLedger.generated.h"
class UMarau6InventoryComponent;
class AMarau6LootActor;
enum class EMarau6Custody : uint8 { Inventory, World, Result, Recovery, Transfer };
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
    FGuid RegisterInventory(UMarau6InventoryComponent* Component);
    void RegisterSeed(AMarau6LootActor* Actor, EMarau6ItemKind Kind);
    bool Pickup(UMarau6InventoryComponent* Component, AMarau6LootActor* Actor, FGuid Expected);
    bool Move(UMarau6InventoryComponent* Component, FGuid Expected, int32 Destination);
    bool Drop(UMarau6InventoryComponent* Component, FGuid Expected);
    void SaveAmmo(UMarau6InventoryComponent* Component, int32 Magazine, int32 Reserve);
    void Finalize(UMarau6InventoryComponent* Component, bool bExtracted);
    void LootRemoved(AMarau6LootActor* Actor, FGuid Id);
    TArray<FMarau6Item> GetHeldResult(FGuid OwnerId) const;
    int32 GetItemCount() const { return Records.Num(); }
    int32 GetRecoveryCount() const;
    bool IsInventoryBusy(FGuid Owner) const { return BusyOwners.Contains(Owner); }
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMarau6ItemLedger,STATGROUP_Tickables); }
private:
    friend class UMarau6InventoryComponent;
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
