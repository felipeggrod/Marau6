#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Marau6InventoryTypes.h"
#include "Marau6InventoryComponent.generated.h"
class AMarau6LootActor;
class AMarau6Character;
UCLASS()
class UMarau6InventoryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UMarau6InventoryComponent();
    void InitializeForRaid();
    bool SaveEquippedShot();
    bool SaveEquippedAmmo(int32 Magazine,int32 Reserve);
    bool FinalizeRaid(bool bExtracted);
    void ApplySnapshot(const FMarau6InventorySnapshot& Value);
    AMarau6LootActor* LookedAtLoot() const;
    const FMarau6InventorySnapshot& GetSnapshot() const { return Snapshot; }
    const FMarau6Item* ItemAt(int32 Slot) const;
    bool HasRifle() const;
    FGuid GetSessionInventoryId() const { return InventoryId; }
    AMarau6Character* Character() const;
    bool CanInteract() const;
    UFUNCTION(Server,Reliable) void ServerPickup(AMarau6LootActor* Loot, FGuid ExpectedId);
    UFUNCTION(Server,Reliable) void ServerMove(FGuid ExpectedId,int32 Destination);
    UFUNCTION(Server,Reliable) void ServerDrop(FGuid ExpectedId);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    friend class UMarau6ItemLedger;
    FGuid InventoryId;
    UPROPERTY(ReplicatedUsing=OnRep_Snapshot) FMarau6InventorySnapshot Snapshot;
    UPROPERTY(ReplicatedUsing=OnRep_Equipment) FGuid EquippedId;
    UFUNCTION() void OnRep_Snapshot();
    UFUNCTION() void OnRep_Equipment();
    virtual void EndPlay(EEndPlayReason::Type Reason) override;
};
