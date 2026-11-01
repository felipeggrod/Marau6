#include "Marau6ItemLedger.h"
#include "Marau6Persistence.h"
#include "EngineUtils.h"
#include "Misc/ScopeExit.h"
#include "Marau6InventoryComponent.h"
#include "Marau6LootActor.h"
#include "Marau6Character.h"
#include "Marau6HealingComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

bool UMarau6ItemLedger::DoesSupportWorldType(EWorldType::Type Type) const { return Type==EWorldType::Game || Type==EWorldType::PIE; }
bool UMarau6ItemLedger::IsServer() const { return GetWorld() && GetWorld()->IsGameWorld() && GetWorld()->GetNetMode()!=NM_Client; }
bool UMarau6ItemLedger::Usable(UMarau6InventoryComponent* C) const
{
    return (!UMarau6Persistence::UsesPersistentBase(GetWorld()) || GetWorld()->GetSubsystem<UMarau6Persistence>()->CanMutate()) && IsServer() && IsValid(C) && !BusyOwners.Contains(C->GetSessionInventoryId()) && C->CanInteract() && !Finalized.Contains(C->GetSessionInventoryId())
        && Owners.FindRef(C->GetSessionInventoryId()).Get()==C;
}
FMarau6ItemRecord* UMarau6ItemLedger::Owned(UMarau6InventoryComponent* C,FGuid Id)
{
    auto* R=Records.Find(Id);
    return R && R->Custody==EMarau6Custody::Inventory && R->OwnerId==C->GetSessionInventoryId() ? R : nullptr;
}
FMarau6ItemRecord* UMarau6ItemLedger::At(FGuid Owner,int32 Slot)
{
    for(auto& Pair:Records) if((Pair.Value.Custody==EMarau6Custody::Inventory || Pair.Value.Custody==EMarau6Custody::Transfer) && Pair.Value.OwnerId==Owner && Pair.Value.Item.Slot==Slot) return &Pair.Value;
    return nullptr;
}
int32 UMarau6ItemLedger::Empty(FGuid Owner,int32 Start,int32 Count)
{
    for(int32 Slot=Start;Slot<Start+Count;++Slot) if(!At(Owner,Slot)) return Slot;
    return INDEX_NONE;
}
void UMarau6ItemLedger::Sync(FGuid Owner)
{
    auto* C=Owners.FindRef(Owner).Get(); if(!C) return;
    FMarau6InventorySnapshot S; S.bFinalized=Finalized.Contains(Owner); S.LostCount=LostCounts.FindRef(Owner);
    for(const auto& Pair:Records)
        if(Pair.Value.OwnerId==Owner && (Pair.Value.Custody==EMarau6Custody::Inventory || Pair.Value.Custody==EMarau6Custody::Result)) S.Items.Add(Pair.Value.Item);
    S.Items.Sort([](const FMarau6Item& A,const FMarau6Item& B){return A.Slot<B.Slot;});
    C->ApplySnapshot(S);
}
FGuid UMarau6ItemLedger::RegisterInventory(UMarau6InventoryComponent* C)
{
    if(!IsServer() || !IsValid(C)) return FGuid();
    const FGuid Owner=FGuid::NewGuid(); Owners.Add(Owner,C);
    FMarau6ItemRecord R; R.Item.Id=FGuid::NewGuid(); R.Item.Kind=EMarau6ItemKind::Rifle;
    R.Item.Slot=Marau6Slots::Weapon; R.Item.Magazine=20; R.Item.Reserve=40; R.OwnerId=Owner;
    Records.Add(R.Item.Id,R); return Owner;
}
void UMarau6ItemLedger::RegisterSeed(AMarau6LootActor* Actor,EMarau6ItemKind Kind)
{
    if(!IsServer() || !IsValid(Actor) || Actor->GetItem().Id.IsValid()) return;
    FMarau6ItemRecord R; R.Item.Id=FGuid::NewGuid(); R.Item.Kind=Kind;
    if(R.Item.IsRifle()){R.Item.Magazine=20;R.Item.Reserve=40;}
    R.Custody=EMarau6Custody::World; R.Loot=Actor; Records.Add(R.Item.Id,R); Actor->SetItem(R.Item);
}
bool UMarau6ItemLedger::Pickup(UMarau6InventoryComponent* C,AMarau6LootActor* Actor,FGuid Expected)
{
    if(!Usable(C) || !IsValid(Actor) || Actor->IsActorBeingDestroyed() || !Expected.IsValid()) return false;
    auto* R=Records.Find(Expected);
    if(!R || R->Custody!=EMarau6Custody::World || R->Loot.Get()!=Actor || Actor->GetItem().Id!=Expected) return false;
    auto* Pawn=C->Character();
    if(C->LookedAtLoot()!=Actor) return false;
    if(FVector::DistSquared(Pawn->GetActorLocation(),Actor->GetActorLocation())>FMath::Square(250.f)) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(Marau6Pickup),false,Pawn); FHitResult Hit;
    if(GetWorld()->LineTraceSingleByChannel(Hit,Pawn->GetPawnViewLocation(),Actor->GetActorLocation(),ECC_Visibility,Query)
        && Hit.GetActor()!=Actor) return false;
    const FGuid Owner=C->GetSessionInventoryId();
    const int32 Slot=R->Item.IsRifle() && !At(Owner,Marau6Slots::Weapon) ? Marau6Slots::Weapon : Empty(Owner,0,Marau6Slots::BackpackCount);
    if(Slot==INDEX_NONE) return false;
    const auto Before=*R;
    R->Custody=EMarau6Custody::Inventory; R->OwnerId=Owner; R->Item.Slot=Slot; R->Loot.Reset();
    if(!Persist()){*R=Before;return false;}
    Actor->Consume(); Sync(Owner); return true;
}
bool UMarau6ItemLedger::Move(UMarau6InventoryComponent* C,FGuid Expected,int32 Destination)
{
    if(!Usable(C)) return false;
    auto* R=Owned(C,Expected); if(!R || R->Item.Slot==Destination) return false;
    auto* Other=At(C->GetSessionInventoryId(),Destination);
    if(R->Item.IsAmmo() && Other && Other->Item.IsRifle())
    {
        auto* Pawn=C->Character();
        if(Other->Custody!=EMarau6Custody::Inventory || Other->Item.Reserve>Marau6Ammo::MaximumReserve-Marau6Ammo::BoxRounds ||
           Pawn->GetHealing()->IsHealing() || (Destination==Marau6Slots::Weapon && Pawn->IsReloading()))return false;
        const auto Before=*R;const auto RifleBefore=Other->Item;
        R->Custody=EMarau6Custody::Consumed;R->OwnerId.Invalidate();R->Item.Slot=INDEX_NONE;R->Loot.Reset();
        Other->Item.Reserve+=Marau6Ammo::BoxRounds;
        if(!Persist()){*R=Before;Other->Item=RifleBefore;return false;}
        Sync(C->GetSessionInventoryId());
        Pawn->RefreshEquippedAmmo();return true;
    }
    if(!Marau6Slots::Accepts(Destination,R->Item))return false;
    const int32 Source=R->Item.Slot;
    if(Other && (Other->Custody!=EMarau6Custody::Inventory || !Marau6Slots::Accepts(Source,Other->Item))) return false;
    if(Source==Marau6Slots::Weapon || Destination==Marau6Slots::Weapon) C->Character()->CancelReload();
    const int32 OldOther=Other?Other->Item.Slot:INDEX_NONE;
    if(Other) Other->Item.Slot=Source;
    R->Item.Slot=Destination;
    if(!Persist()){R->Item.Slot=Source;if(Other)Other->Item.Slot=OldOther;return false;}
    Sync(C->GetSessionInventoryId()); return true;
}
bool UMarau6ItemLedger::SpawnWorld(FGuid Id,FVector Location)
{
    if(!IsServer() || GetWorld()->bIsTearingDown) return false;
    const auto* Current=Records.Find(Id);
    if(!Current || (Current->Custody!=EMarau6Custody::Inventory && Current->Custody!=EMarau6Custody::Recovery)) return false;
    const FMarau6ItemRecord Before=*Current;
    // Reserve before any spawn callback; never keep a TMap element reference
    // across actor creation, destruction, or user/gameplay delegates.
    Records.FindChecked(Id).Custody=EMarau6Custody::Transfer;
    const bool bOwnsReservation=Before.OwnerId.IsValid() && !BusyOwners.Contains(Before.OwnerId);
    if(bOwnsReservation) BusyOwners.Add(Before.OwnerId);
    ON_SCOPE_EXIT { if(bOwnsReservation) BusyOwners.Remove(Before.OwnerId); };
    FTransform Transform(FRotator::ZeroRotator,Location);
    FMarau6Item Item=Before.Item;Item.Slot=INDEX_NONE;
    FActorSpawnParameters Spawn;Spawn.bDeferConstruction=true;
    Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Spawn.CustomPreSpawnInitialization=[Item](AActor* NewActor){CastChecked<AMarau6LootActor>(NewActor)->SetItem(Item);};
    auto* Actor=GetWorld()->SpawnActor<AMarau6LootActor>(AMarau6LootActor::StaticClass(),Location,FRotator::ZeroRotator,Spawn);
    if(IsValid(Actor) && !Actor->IsActorBeingDestroyed())
    {
        Actor->SetItem(Item);UGameplayStatics::FinishSpawningActor(Actor,Transform);
    }
    if(IsValid(Actor) && !Actor->IsActorBeingDestroyed())
    {
        auto* Record=Records.Find(Id);
        if(Record && Record->Custody==EMarau6Custody::Transfer)
        {
            Record->Item=Item;Record->Custody=EMarau6Custody::World;Record->OwnerId.Invalidate();Record->Loot=Actor;
            return true;
        }
    }
    if(IsValid(Actor) && !Actor->IsActorBeingDestroyed()) Actor->Consume();
    if(auto* Record=Records.Find(Id);Record && Record->Custody==EMarau6Custody::Transfer)
    {
        *Record=Before;
        if(Before.Custody==EMarau6Custody::Inventory && Finalized.Contains(Before.OwnerId))
        {
            Record->Custody=ExtractedOwners.Contains(Before.OwnerId)?EMarau6Custody::Result:EMarau6Custody::Recovery;
            Record->RecoveryLocation=Location;
        }
    }
    return false;
}
bool UMarau6ItemLedger::Drop(UMarau6InventoryComponent* C,FGuid Expected)
{
    if(!Usable(C)) return false;
    auto* R=Owned(C,Expected); if(!R) return false;
    auto* Pawn=C->Character();
    if(R->Item.Slot==Marau6Slots::Weapon) Pawn->CancelReload();
    FVector Point=Pawn->GetActorLocation()+Pawn->GetActorForwardVector()*85.f;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(Marau6Drop),false,Pawn);FHitResult Hit;
    if(GetWorld()->LineTraceSingleByChannel(Hit,Pawn->GetPawnViewLocation(),Point,ECC_Visibility,Query)) Point=Hit.ImpactPoint-Pawn->GetActorForwardVector()*25.f;
    if(GetWorld()->LineTraceSingleByChannel(Hit,Point,Point-FVector(0,0,500),ECC_Visibility,Query)) Point=Hit.ImpactPoint+FVector(0,0,25);
    const FGuid Owner=C->GetSessionInventoryId();
    const auto Before=Records;
    bool bTransferred=SpawnWorld(Expected,Point);
    if(bTransferred && !Persist())
    {if(auto* Created=Records.Find(Expected);Created && Created->Loot.IsValid())Created->Loot->Consume();Records=Before;bTransferred=false;}
    Sync(Owner);return bTransferred;
}
bool UMarau6ItemLedger::SaveShot(UMarau6InventoryComponent* C)
{
    if(!Usable(C))return false;
    auto* R=At(C->GetSessionInventoryId(),Marau6Slots::Weapon);
    if(!R || R->Custody!=EMarau6Custody::Inventory || !R->Item.IsRifle() || R->Item.Durability<=0 || R->Item.Magazine<=0)return false;
    const FMarau6Item Before=R->Item;
    --R->Item.Magazine;R->Item.Durability=FMath::Max(0,R->Item.Durability-Marau6Ammo::ShotWear);
    if(!Persist()){R->Item=Before;return false;}
    Sync(C->GetSessionInventoryId());return true;
}
bool UMarau6ItemLedger::SaveAmmo(UMarau6InventoryComponent* C,int32 Magazine,int32 Reserve)
{
    if(!Usable(C)) return false;
    if(auto* R=At(C->GetSessionInventoryId(),Marau6Slots::Weapon))
    {
        const auto Before=R->Item;
        R->Item.Magazine=FMath::Clamp(Magazine,0,Marau6Ammo::Magazine);R->Item.Reserve=FMath::Clamp(Reserve,0,Marau6Ammo::MaximumReserve);
        if(!Persist()){R->Item=Before;return false;}
        Sync(C->GetSessionInventoryId());return true;
    }
    return false;
}
bool UMarau6ItemLedger::Finalize(UMarau6InventoryComponent* C,bool bExtracted)
{
    if(!IsServer() || !IsValid(C)) return false;
    const FGuid Owner=C->GetSessionInventoryId();
    if(!Owner.IsValid() || Owners.FindRef(Owner).Get()!=C) return false;
    if(Finalized.Contains(Owner)) return true;
    if(UMarau6Persistence::UsesPersistentBase(GetWorld()))
    {
        auto* Bridge=GetWorld()->GetSubsystem<UMarau6Persistence>();
        if(Bridge->IsEntryPending())
        {if(!bExtracted)Bridge->DeferDeath(Owner,C->GetOwner()->GetActorLocation());return false;}
    }
    const auto Before=Records;
    C->Character()->CancelReload(); Finalized.Add(Owner);
    if(bExtracted) ExtractedOwners.Add(Owner);
    TArray<FGuid> Ids;
    for(const auto& Pair:Records)
        if(Pair.Value.OwnerId==Owner && Pair.Value.Custody==EMarau6Custody::Inventory) Ids.Add(Pair.Key);
    int32 Lost=0;
    for(FGuid Id:Ids)
    {
        auto* R=Records.Find(Id);
        if(!R || R->OwnerId!=Owner || R->Custody!=EMarau6Custody::Inventory) continue;
        if(bExtracted || Marau6Slots::IsSafe(R->Item.Slot)) R->Custody=EMarau6Custody::Result;
        else
        {
            const float Angle=Lost*1.9f;
            const FVector Point=C->GetOwner()->GetActorLocation()+FVector(FMath::Cos(Angle)*65,FMath::Sin(Angle)*65,-60);
            ++Lost;
            if(!SpawnWorld(Id,Point))
                if(auto* Pending=Records.Find(Id)) {Pending->Custody=EMarau6Custody::Recovery;Pending->RecoveryLocation=Point;Pending->Loot.Reset();}
        }
    }
    LostCounts.Add(Owner,Lost);
    if(!Persist())
    {
        TArray<TWeakObjectPtr<AMarau6LootActor>> Created;
        for(const auto& Pair:Records)if(Pair.Value.Loot.IsValid() && Before.Contains(Pair.Key) && Pair.Value.Loot!=Before.FindChecked(Pair.Key).Loot)Created.Add(Pair.Value.Loot);
        for(auto A:Created)if(A.IsValid())A->Consume();
        Records=Before;Finalized.Remove(Owner);ExtractedOwners.Remove(Owner);LostCounts.Remove(Owner);return false;
    }
    Sync(Owner);return true;
}
void UMarau6ItemLedger::LootRemoved(AMarau6LootActor* Actor,FGuid Id)
{
    if(!IsServer()) return;
    if(auto* R=Records.Find(Id); R && R->Custody==EMarau6Custody::World && R->Loot.Get()==Actor)
    {R->Custody=EMarau6Custody::Recovery;R->RecoveryLocation=Actor->GetActorLocation();R->Loot.Reset();}
}
TArray<FMarau6Item> UMarau6ItemLedger::GetHeldResult(FGuid Owner) const
{
    TArray<FMarau6Item> Out;
    for(const auto& Pair:Records) if(Pair.Value.OwnerId==Owner && Pair.Value.Custody==EMarau6Custody::Result) Out.Add(Pair.Value.Item);
    return Out;
}
int32 UMarau6ItemLedger::GetRecoveryCount() const
{
    int32 Count=0;for(const auto& Pair:Records) if(Pair.Value.Custody==EMarau6Custody::Recovery) ++Count;return Count;
}

void UMarau6ItemLedger::Tick(float DeltaTime)
{
    if(!IsServer() || GetWorld()->bIsTearingDown) return;
    RecoverySeconds+=DeltaTime;if(RecoverySeconds<1.f) return;RecoverySeconds=0.f;
    TArray<FGuid> Pending;
    for(const auto& Pair:Records) if(Pair.Value.Custody==EMarau6Custody::Recovery) Pending.Add(Pair.Key);
    for(FGuid Id:Pending)
    {
        const auto* R=Records.Find(Id);if(!R || R->Custody!=EMarau6Custody::Recovery) continue;
        const FVector Location=R->RecoveryLocation;SpawnWorld(Id,Location);
    }
}

bool UMarau6ItemLedger::Persist()
{
    if(!UMarau6Persistence::UsesPersistentBase(GetWorld())) return true;
    return GetWorld()->GetSubsystem<UMarau6Persistence>()->CommitLedger();
}
bool UMarau6ItemLedger::Import(UMarau6InventoryComponent* C,FGuid Owner,const TArray<FMarau6Item>& Items)
{
    if(!IsServer() || !IsValid(C) || !Owner.IsValid()) return false;
    TSet<FGuid> Ids;TSet<int32> Slots;
    for(const auto& I:Items)
    {if(!I.Id.IsValid() || Ids.Contains(I.Id) || Slots.Contains(I.Slot) || !Marau6Slots::Accepts(I.Slot,I) || (I.IsRifle() && (I.Durability<0 || I.Durability>10000)) || Records.Contains(I.Id))return false;Ids.Add(I.Id);Slots.Add(I.Slot);}
    const FGuid Previous=C->InventoryId;
    TArray<FGuid> Old;for(const auto& Pair:Records)if(Pair.Value.OwnerId==Previous && Pair.Value.Custody==EMarau6Custody::Inventory)Old.Add(Pair.Key);
    for(FGuid Id:Old)Records.Remove(Id);Owners.Remove(Previous);
    C->InventoryId=Owner;Owners.Add(Owner,C);Finalized.Remove(Owner);ExtractedOwners.Remove(Owner);LostCounts.Remove(Owner);
    for(const auto& I:Items){FMarau6ItemRecord R;R.Item=I;R.OwnerId=Owner;Records.Add(I.Id,R);}
    Sync(Owner);return true;
}
TArray<TSharedPtr<FJsonValue>> UMarau6ItemLedger::Export() const
{
    TArray<TSharedPtr<FJsonValue>> Out;
    TArray<FGuid> Ids;Records.GetKeys(Ids);Ids.Sort([](const FGuid& A,const FGuid& B){return A.ToString()<B.ToString();});
    for(FGuid Id:Ids)
    {
        const auto& R=Records.FindChecked(Id);auto O=MakeShared<FJsonObject>();
        const bool bOwned=R.Custody==EMarau6Custody::Inventory || R.Custody==EMarau6Custody::Result;
        O->SetStringField(TEXT("id"),R.Item.Id.ToString(EGuidFormats::DigitsWithHyphensLower));
        O->SetNumberField(TEXT("kind"),static_cast<int32>(R.Item.Kind));O->SetNumberField(TEXT("slot"),bOwned?R.Item.Slot:-1);
        O->SetNumberField(TEXT("magazine"),R.Item.Magazine);O->SetNumberField(TEXT("reserve"),R.Item.Reserve);
        if(R.Item.IsRifle())O->SetNumberField(TEXT("durability"),R.Item.Durability);
        O->SetStringField(TEXT("owner"),bOwned?R.OwnerId.ToString(EGuidFormats::DigitsWithHyphensLower):TEXT(""));
        O->SetStringField(TEXT("custody"),R.Custody==EMarau6Custody::Consumed?TEXT("consumed"):R.Custody==EMarau6Custody::Result?TEXT("result"):bOwned?TEXT("inventory"):TEXT("world"));
        Out.Add(MakeShared<FJsonValueObject>(O));
    }
    return Out;
}
void UMarau6ItemLedger::ResetRound()
{
    Records.Empty();Owners.Empty();Finalized.Empty();ExtractedOwners.Empty();LostCounts.Empty();BusyOwners.Empty();
    TArray<AMarau6LootActor*> Actors;for(TActorIterator<AMarau6LootActor> It(GetWorld());It;++It)Actors.Add(*It);
    for(auto* A:Actors)A->Consume();
}
bool UMarau6ItemLedger::AbandonReserved(FGuid Owner,const TArray<FMarau6Item>& Items,FVector Location)
{
    if(!IsServer() || !Owner.IsValid() || Finalized.Contains(Owner))return false;
    for(const auto& I:Items)if(Records.Contains(I.Id))return false;
    const auto Before=Records;
    int32 Lost=0;
    for(const auto& I:Items)
    {
        FMarau6ItemRecord R;R.Item=I;
        if(Marau6Slots::IsSafe(I.Slot)){R.OwnerId=Owner;R.Custody=EMarau6Custody::Result;}
        else {R.Custody=EMarau6Custody::Recovery;R.RecoveryLocation=Location+FVector(FMath::Cos(Lost*1.9f)*65,FMath::Sin(Lost*1.9f)*65,-60);++Lost;}
        Records.Add(I.Id,R);
        if(R.Custody==EMarau6Custody::Recovery)SpawnWorld(I.Id,R.RecoveryLocation);
    }
    Finalized.Add(Owner);LostCounts.Add(Owner,Lost);
    if(Persist())return true;
    TArray<TWeakObjectPtr<AMarau6LootActor>> Created;
    for(const auto& I:Items)if(auto* R=Records.Find(I.Id);R && R->Loot.IsValid())Created.Add(R->Loot);
    for(auto A:Created)if(A.IsValid())A->Consume();
    Records=Before;Finalized.Remove(Owner);LostCounts.Remove(Owner);return false;
}


bool UMarau6ItemLedger::FinalizeDisconnected(FGuid Owner,FVector Location)
{
    if(Finalized.Contains(Owner))return true;
    const auto Before=Records;
    TArray<FMarau6Item> Items;
    for(const auto& Pair:Records)if(Pair.Value.OwnerId==Owner && Pair.Value.Custody==EMarau6Custody::Inventory)Items.Add(Pair.Value.Item);
    for(const auto& I:Items)Records.Remove(I.Id);
    if(AbandonReserved(Owner,Items,Location)){Owners.Remove(Owner);return true;}
    Records=Before;return false;
}

bool UMarau6ItemLedger::GrantWorld(FGuid Id,EMarau6ItemKind Kind,FVector Location)
{
    if(!IsServer() || !Id.IsValid() || Location.ContainsNaN() || static_cast<uint8>(Kind)>static_cast<uint8>(EMarau6ItemKind::Ammo))return false;
    if(const auto* Existing=Records.Find(Id))return Existing->Item.Kind==Kind;
    if(UMarau6Persistence::UsesPersistentBase(GetWorld()))
    {auto* Bridge=GetWorld()->GetSubsystem<UMarau6Persistence>();if(!Bridge || !Bridge->CanMutate())return false;}
    FMarau6ItemRecord Record;Record.Item.Id=Id;Record.Item.Kind=Kind;Record.Item.Slot=INDEX_NONE;
    if(Record.Item.IsRifle()){Record.Item.Magazine=20;Record.Item.Reserve=40;}
    Record.Custody=EMarau6Custody::Recovery;Record.RecoveryLocation=Location;
    Records.Add(Id,Record);
    // Durable before visible. Recovery and world actors have the same exported custody.
    if(!Persist()){Records.Remove(Id);return false;}
    SpawnWorld(Id,Location);return true;
}

bool UMarau6ItemLedger::ConsumeForHealing(UMarau6InventoryComponent* C,FGuid Id)
{
    if(!Usable(C))return false;
    auto* Pawn=C->Character();auto* Record=Owned(C,Id);
    if(!Pawn || Pawn->GetRaidState()!=EMarau6RaidState::Active || Pawn->Health>=100 || !Record || Record->Item.Kind!=EMarau6ItemKind::Supply || Record->Item.Slot<0 || Record->Item.Slot>=Marau6Slots::Weapon)return false;
    const FMarau6ItemRecord Before=*Record;
    Record->Custody=EMarau6Custody::Consumed;Record->OwnerId.Invalidate();Record->Item.Slot=INDEX_NONE;Record->Loot.Reset();
    if(!Persist()){Records.FindChecked(Id)=Before;return false;}
    // No actor creation or gameplay delegate occurs before this paid-for health
    // change. Sync callbacks observe both the terminal item and applied health.
    Pawn->Health=FMath::Min(100.f,Pawn->Health+40.f);Pawn->OnRep_Health();Pawn->ForceNetUpdate();
    Sync(C->GetSessionInventoryId());return true;
}
