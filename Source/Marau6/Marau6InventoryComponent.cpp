#include "Marau6InventoryComponent.h"
#include "Marau6Character.h"
#include "Marau6Persistence.h"
#include "Marau6ItemLedger.h"
#include "Marau6LootActor.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

UMarau6InventoryComponent::UMarau6InventoryComponent() { SetIsReplicatedByDefault(true);PrimaryComponentTick.bCanEverTick=false; }
AMarau6Character* UMarau6InventoryComponent::Character() const { return Cast<AMarau6Character>(GetOwner()); }
bool UMarau6InventoryComponent::CanInteract() const { auto* C=Character();return C && C->HasAuthority() && C->CanParticipate() && C->GetController() && !Snapshot.bFinalized; }
void UMarau6InventoryComponent::InitializeForRaid()
{
    if(UMarau6Persistence::UsesPersistentBase(GetWorld()) || InventoryId.IsValid() || !GetWorld() || !GetOwner()->HasAuthority()) return;
    if(auto* L=GetWorld()->GetSubsystem<UMarau6ItemLedger>()) { InventoryId=L->RegisterInventory(this);if(InventoryId.IsValid()) L->Sync(InventoryId); }
}
const FMarau6Item* UMarau6InventoryComponent::ItemAt(int32 Slot) const
{ return Snapshot.Items.FindByPredicate([Slot](const FMarau6Item& Item){return Item.Slot==Slot;}); }
void UMarau6InventoryComponent::ApplySnapshot(const FMarau6InventorySnapshot& Value)
{
    if(!GetOwner()->HasAuthority()) return;
    const FGuid Previous=EquippedId;Snapshot=Value;EquippedId.Invalidate();
    if(!Snapshot.bFinalized) if(const auto* Item=ItemAt(Marau6Slots::Weapon)) EquippedId=Item->Id;
    if(auto* C=Character()){C->OnInventoryChanged(Previous!=EquippedId);if(Snapshot.bFinalized) C->OpenInventoryResult();}
    GetOwner()->ForceNetUpdate();
}
void UMarau6InventoryComponent::OnRep_Snapshot()
{ if(auto* C=Character()){C->OnInventoryChanged(false);if(Snapshot.bFinalized) C->OpenInventoryResult();} }
void UMarau6InventoryComponent::OnRep_Equipment() { if(auto* C=Character()) C->OnInventoryChanged(false); }
bool UMarau6InventoryComponent::SaveEquippedAmmo(int32 Magazine,int32 Reserve)
{ if(auto* L=GetWorld()->GetSubsystem<UMarau6ItemLedger>()) return L->SaveAmmo(this,Magazine,Reserve); return false; }
bool UMarau6InventoryComponent::FinalizeRaid(bool bExtracted)
{ if(auto* L=GetWorld()->GetSubsystem<UMarau6ItemLedger>()) return L->Finalize(this,bExtracted); return false; }
AMarau6LootActor* UMarau6InventoryComponent::LookedAtLoot() const
{
    auto* C=Character();if(!C || !C->GetWorld() || !C->CanParticipate()) return nullptr;
    const FVector Start=C->GetPawnViewLocation();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(Marau6LootAim),false,C);FHitResult Hit;
    C->GetWorld()->LineTraceSingleByChannel(Hit,Start,Start+C->GetControlRotation().Vector()*250.f,ECC_Visibility,Query);
    return Cast<AMarau6LootActor>(Hit.GetActor());
}
void UMarau6InventoryComponent::ServerPickup_Implementation(AMarau6LootActor* Loot,FGuid ExpectedId)
{ if(auto* L=GetWorld()->GetSubsystem<UMarau6ItemLedger>()) L->Pickup(this,Loot,ExpectedId); }
void UMarau6InventoryComponent::ServerMove_Implementation(FGuid ExpectedId,int32 Destination)
{ if(auto* L=GetWorld()->GetSubsystem<UMarau6ItemLedger>()) L->Move(this,ExpectedId,Destination); }
void UMarau6InventoryComponent::ServerDrop_Implementation(FGuid ExpectedId)
{ if(auto* L=GetWorld()->GetSubsystem<UMarau6ItemLedger>()) L->Drop(this,ExpectedId); }
void UMarau6InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME_CONDITION(UMarau6InventoryComponent,Snapshot,COND_OwnerOnly);DOREPLIFETIME(UMarau6InventoryComponent,EquippedId);
}
void UMarau6InventoryComponent::EndPlay(EEndPlayReason::Type Reason)
{
    if(Reason==EEndPlayReason::Destroyed && GetWorld() && !GetWorld()->bIsTearingDown && GetOwner()->HasAuthority() && !Snapshot.bFinalized) FinalizeRaid(false);
    Super::EndPlay(Reason);
}

bool UMarau6InventoryComponent::HasRifle() const
{
    if(!EquippedId.IsValid()) return false;
    if(GetOwner()->HasAuthority()) if(const auto* L=GetWorld()->GetSubsystem<UMarau6ItemLedger>())
        if(L->IsInventoryBusy(InventoryId)) return false;
    return true;
}
