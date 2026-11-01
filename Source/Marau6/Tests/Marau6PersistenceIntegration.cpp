#if WITH_EDITOR
#include "HAL/IConsoleManager.h"
#include "Marau6Persistence.h"
#include "Marau6PlayerController.h"
#include "Marau6Character.h"
#include "Marau6InventoryComponent.h"
#include "Marau6ItemLedger.h"
#include "Marau6LootActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
struct FCycle
{
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<AMarau6PlayerController> A,B;
    int32 Stage=0;double Started=0;FString PreviousRound;FGuid Rifle,Safe;
    bool bVerify=false,bDisconnectOther=false,bReconnectProbe=false;
    FTimerHandle Timer;
    TSharedPtr<FJsonObject> State(AMarau6PlayerController* C)
    {TSharedPtr<FJsonObject> O;if(C)FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(C->GetBaseJson()),O);return O;}
    TSharedPtr<FJsonObject> Item(AMarau6PlayerController* C,int32 Slot)
    {auto O=State(C);if(!O)return nullptr;for(const auto& V:O->GetArrayField(TEXT("items")))if(V->AsObject()->GetIntegerField(TEXT("slot"))==Slot)return V->AsObject();return nullptr;}
    void Advance(const TCHAR* What){UE_LOG(LogTemp,Display,TEXT("MARAU6_INTEGRATION PASS stage=%d %s"),Stage,What);++Stage;}
    void Error(const TCHAR* What){UE_LOG(LogTemp,Error,TEXT("MARAU6_INTEGRATION FAILED stage=%d %s"),Stage,What);FPlatformMisc::RequestExitWithStatus(false,1);}
    void Tick()
    {
        auto* W=World.Get();if(!W)return;
        if(FPlatformTime::Seconds()-Started>180){Error(TEXT("timeout"));return;}
        auto* P=W->GetSubsystem<UMarau6Persistence>();auto* L=W->GetSubsystem<UMarau6ItemLedger>();
        auto* CA=A.Get();auto* CB=B.Get();
        auto* PA=CA?Cast<AMarau6Character>(CA->GetPawn()):nullptr;
        auto* PB=CB?Cast<AMarau6Character>(CB->GetPawn()):nullptr;
        if(Stage==0)
        {
            if(!P->CanMutate() || !CA || !CB)return;
            P->SelectProfile(CA,1);P->SelectProfile(CB,2);Advance(TEXT("select profiles"));return;
        }
        if((Stage<11 && !PA) || (Stage<11 && !PB)){Error(TEXT("missing pawn"));return;}
        if(bVerify)
        {
            if(!State(CA) || !State(CB))return;
            if(!Item(CA,8) || Item(CB,10) || Item(CB,0)){Error(TEXT("abandonment or safe recovery incorrect"));return;}
            UE_LOG(LogTemp,Display,TEXT("MARAU6_INTEGRATION ALL PASSED recovery"));FPlatformMisc::RequestExit(false);return;
        }
        switch(Stage)
        {
        case 1:
            if(!Item(CA,100) || !State(CB))return;
            P->MoveBase(CA,100,8);Advance(TEXT("prepare safe"));break;
        case 2:
            if(!Item(CA,8))return;
            FGuid::Parse(Item(CA,8)->GetStringField(TEXT("id")),Safe);
            P->EnterRaid(CA);Advance(TEXT("enter A"));break;
        case 3:
            if(!CA->IsInRaid())return;
            P->EnterRaid(CB);Advance(TEXT("enter B"));break;
        case 4:
            if(!CB->IsInRaid() || !P->CanMutate())return;
            if(!PA->GetInventory()->ItemAt(10)){Error(TEXT("missing reserved rifle"));return;}
            Rifle=PA->GetInventory()->ItemAt(10)->Id;
            PA->ProcessEvent(PA->FindFunction(TEXT("ServerFire")),nullptr);
            if(PA->GetMagazineAmmo()!=19 || PA->GetRifleDurability()!=9900){Error(TEXT("server fire not saved"));return;}
            if(!L->Drop(PA->GetInventory(),Rifle)){Error(TEXT("drop failed"));return;}
            Advance(TEXT("fired and dropped rifle"));break;
        case 5:
            {
                AMarau6LootActor* Found=nullptr;for(TActorIterator<AMarau6LootActor> It(W);It;++It)if(It->GetItem().Id==Rifle)Found=*It;
                if(!Found){Error(TEXT("dropped rifle missing"));return;}
                PB->SetActorLocation(Found->GetActorLocation()+FVector(-100,0,96));
                CB->SetControlRotation((Found->GetActorLocation()-PB->GetPawnViewLocation()).Rotation());
                if(!L->Pickup(PB->GetInventory(),Found,Rifle)){Error(TEXT("pickup failed"));return;}
                PA->TakeDamage(100,FDamageEvent(),CB,PB);
                if(PA->IsAlive()){Error(TEXT("death failed"));return;}
                Advance(TEXT("transfer rifle and eliminate A"));
            }break;
        case 6:
            if(CA->IsInRaid() || !Item(CA,8))return;
            if(Item(CA,10) || Item(CA,8)->GetStringField(TEXT("id"))!=Safe.ToString(EGuidFormats::DigitsWithHyphensLower)){Error(TEXT("death safe mismatch"));return;}
            PB->SetActorLocation(FVector(1400,0,96));Advance(TEXT("death committed and extracting B"));break;
        case 7:
            if(CB->IsInRaid() || !Item(CB,0))return;
            if(Item(CB,0)->GetStringField(TEXT("id"))!=Rifle.ToString(EGuidFormats::DigitsWithHyphensLower) || Item(CB,0)->GetIntegerField(TEXT("magazine"))!=19 || Item(CB,0)->GetIntegerField(TEXT("durability"))!=9900){Error(TEXT("extracted transfer mismatch"));return;}
            if(!bReconnectProbe)
            {
                UClass* ControllerClass=CA->GetClass();UClass* PawnClass=PA->GetClass();
                P->Disconnect(CA);CA->Destroy();
                auto* NewController=W->SpawnActor<AMarau6PlayerController>(ControllerClass);
                FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                auto* NewPawn=W->SpawnActor<AMarau6Character>(PawnClass,FVector(0,400,600),FRotator::ZeroRotator,Spawn);
                NewController->Possess(NewPawn);A=NewController;bReconnectProbe=true;P->SelectProfile(NewController,1);return;
            }
            if(!Item(CA,8))return;
            P->EnterRaid(CA);
            if(CA->IsInRaid() || PA->CanParticipate()){Error(TEXT("reconnect bypasses finalized participation"));return;}
            PreviousRound=P->GetRoundId();P->NextRound(CB);Advance(TEXT("reconnect blocked; extraction committed and next round"));break;
        case 8:
            if(P->GetRoundId()==PreviousRound || !P->CanMutate())return;
            P->EnterRaid(CB);Advance(TEXT("reenter with retained gear"));break;
        case 9:
            if(!CB->IsInRaid() || !P->CanMutate())return;
            if(!L->Move(PB->GetInventory(),Rifle,10)){Error(TEXT("equip retained rifle"));return;}
            PB->ProcessEvent(PB->FindFunction(TEXT("ServerFire")),nullptr);
            if(PB->GetMagazineAmmo()!=18 || PB->GetRifleDurability()!=9800){Error(TEXT("retained ammo not usable"));return;}
            Advance(TEXT("second raid fire committed"));break;
        case 10:
                        if(!P->CanMutate())return;
            P->EnterRaid(CA);if(bDisconnectOther){P->Disconnect(CB);CB->Destroy();}else{P->Disconnect(CA);CA->Destroy();}
            Advance(TEXT("disconnect during reservation"));break;
        case 11:
            if(P->GetStatus().Contains(TEXT("desconectou durante")) || P->GetStatus().Contains(TEXT("rejeitou operacao"))){Error(TEXT("reservation disconnect froze other player"));return;}
            {FGuid Owner;FGuid::Parse(bDisconnectOther?TEXT("22222222-2222-2222-2222-222222222222"):TEXT("11111111-1111-1111-1111-111111111111"),Owner);if(!P->CanMutate() || !P->IsSettled() || !L->IsFinalized(Owner))return;}
            if(!(bDisconnectOther?PA:PB) || !(bDisconnectOther?PA:PB)->CanParticipate()){Error(TEXT("surviving player blocked"));return;}
            // Leave the second participation open deliberately. Next process must recover
            // the journal then abandon it, without returning equipment to B.
            if(!P->CanMutate())return;
            UE_LOG(LogTemp,Display,TEXT("MARAU6_INTEGRATION ALL PASSED cycle"));FPlatformMisc::RequestExit(false);break;
        }
    }
};
FAutoConsoleCommandWithWorldAndArgs RunPersistenceCycle(TEXT("Marau6.TestPersistenceCycle"),TEXT("Editor integration test; requires -Marau6Integration and isolated test backend."),
FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args,UWorld* W)
{
    if(!W || !W->IsGameWorld() || !FParse::Param(FCommandLine::Get(),TEXT("Marau6Integration")))return;
    AMarau6PlayerController* First=nullptr;for(TActorIterator<AMarau6PlayerController> It(W);It;++It)if(It->GetPawn()){First=*It;break;}
    if(!First){UE_LOG(LogTemp,Error,TEXT("MARAU6_INTEGRATION no player"));return;}
    auto* Second=W->SpawnActor<AMarau6PlayerController>(First->GetClass());
    FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Pawn=W->SpawnActor<AMarau6Character>(First->GetPawn()->GetClass(),FVector(0,300,120),FRotator::ZeroRotator,Spawn);Second->Possess(Pawn);
    auto State=MakeShared<FCycle>();State->World=W;State->A=First;State->B=Second;State->Started=FPlatformTime::Seconds();State->bVerify=Args.Contains(TEXT("verify"));State->bDisconnectOther=Args.Contains(TEXT("other"));
    W->GetTimerManager().SetTimer(State->Timer,FTimerDelegate::CreateLambda([State]{State->Tick();}),.2f,true);
}));
}
#endif
