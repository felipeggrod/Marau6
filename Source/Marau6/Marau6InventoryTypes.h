#pragma once
#include "CoreMinimal.h"
#include "Marau6InventoryTypes.generated.h"
UENUM()
enum class EMarau6ItemKind : uint8 { Rifle, Supply, Scrap };
USTRUCT()
struct FMarau6Item
{
    GENERATED_BODY()
    UPROPERTY() FGuid Id;
    UPROPERTY() EMarau6ItemKind Kind = EMarau6ItemKind::Supply;
    UPROPERTY() int32 Slot = INDEX_NONE;
    UPROPERTY() int32 Magazine = 0;
    UPROPERTY() int32 Reserve = 0;
    // Fixed-point: 10000 = 100%, one accepted shot costs 250 = 2.5 points.
    UPROPERTY() int32 Durability = 10000;
    bool IsRifle() const { return Kind == EMarau6ItemKind::Rifle; }
    FString Name() const { return IsRifle() ? TEXT("Rifle") : Kind == EMarau6ItemKind::Supply ? TEXT("Suprimentos") : TEXT("Pecas"); }
};
USTRUCT()
struct FMarau6InventorySnapshot
{
    GENERATED_BODY()
    UPROPERTY() TArray<FMarau6Item> Items;
    UPROPERTY() bool bFinalized = false;
    UPROPERTY() int32 LostCount = 0;
};
namespace Marau6Slots
{
    constexpr int32 BackpackCount=8, SafeStart=8, SafeCount=2, Weapon=10, Total=11;
    inline bool IsSafe(int32 Slot) { return Slot>=SafeStart && Slot<Weapon; }
    inline bool Accepts(int32 Slot, const FMarau6Item& Item)
    {
        return Slot>=0 && Slot<Total && (Slot==Weapon ? Item.IsRifle() : !IsSafe(Slot) || !Item.IsRifle());
    }
}
