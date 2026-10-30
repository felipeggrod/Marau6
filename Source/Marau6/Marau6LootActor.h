#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Marau6InventoryTypes.h"
#include "Marau6LootActor.generated.h"
class UStaticMesh;
class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
UCLASS()
class AMarau6LootActor : public AActor
{
    GENERATED_BODY()
public:
    AMarau6LootActor();
    const FMarau6Item& GetItem() const { return Item; }
    void Consume();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    friend class UMarau6ItemLedger;
    void SetItem(const FMarau6Item& Value);
    UPROPERTY(EditAnywhere,Category="Loot") EMarau6ItemKind SeedKind=EMarau6ItemKind::Supply;
    UPROPERTY(ReplicatedUsing=OnRep_Item) FMarau6Item Item;
    UPROPERTY() TObjectPtr<UBoxComponent> Bounds;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY() TObjectPtr<UStaticMesh> RifleMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> BoxMesh;
    UFUNCTION() void OnRep_Item();
    virtual void BeginPlay() override;
    virtual void EndPlay(EEndPlayReason::Type Reason) override;
};
