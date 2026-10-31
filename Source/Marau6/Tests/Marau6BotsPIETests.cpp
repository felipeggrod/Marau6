#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "UObject/StrongObjectPtr.h"
#include "Marau6Bot.h"
#include "Marau6Bots.h"
#include "Marau6Character.h"
#include "Marau6PlayerController.h"
#include "Marau6InventoryComponent.h"
#include "Marau6Persistence.h"
#include "Marau6LootActor.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/DamageEvents.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
struct FBotsPIEData
{
    FAutomationTestBase* Test;
    TStrongObjectPtr<ULevelEditorPlaySettings> Settings;
    explicit FBotsPIEData(FAutomationTestBase* T):Test(T),Settings(DuplicateObject(GetMutableDefault<ULevelEditorPlaySettings>(),GetTransientPackage())){}
};
class FBotsPIEFlow : public IAutomationLatentCommand
{
    TSharedPtr<FBotsPIEData> Data;
    int32 Phase=0,Fired=0,Winner=0;
    double Started=FPlatformTime::Seconds(),At=Started,NextFire=0;
    TMap<FGuid,FVector> Originals;
    TWeakObjectPtr<AMarau6Bot> Victim;
    FGuid Reward;
    float BeforeHealth=100;
    FString Round;
    void Next(){UE_LOG(LogTemp,Display,TEXT("MARAU6_BOTS_PIE completed phase %d"),Phase);++Phase;At=FPlatformTime::Seconds();Data->Test->AddInfo(FString::Printf(TEXT("Bots PIE phase %d"),Phase));}
public:
    explicit FBotsPIEFlow(TSharedPtr<FBotsPIEData> In):Data(In){}
    virtual bool Update()override
    {
        const double Now=FPlatformTime::Seconds();
        if(Now-Started>100){Data->Test->AddError(FString::Printf(TEXT("Bots PIE timeout phase %d"),Phase));return true;}
        UWorld* Server=nullptr;TArray<UWorld*> Clients;
        for(const auto& Context:GEngine->GetWorldContexts())if(auto* W=Context.World();W && W->WorldType==EWorldType::PIE)
        {if(W->GetNetMode()==NM_DedicatedServer)Server=W;else if(W->GetNetMode()==NM_Client)Clients.Add(W);}
        if(!Server || Clients.Num()!=2)return false;
        TArray<AMarau6Bot*> Bots;for(TActorIterator<AMarau6Bot> It(Server);It;++It)Bots.Add(*It);
        auto* Bridge=Server->GetSubsystem<UMarau6Persistence>();
        if(Phase==0)
        {
            if(!Bridge->CanMutate() || Bots.Num()!=3)return false;
            int32 N=0;for(auto* W:Clients)for(TActorIterator<AMarau6PlayerController> It(W);It;++It)if(It->IsLocalController())It->ServerSelectProfile(++N);
            for(auto* B:Bots){Originals.Add(B->GetLootId(),B->GetActorLocation());auto* C=B->GetCapsuleComponent();FCollisionQueryParams Q(SCENE_QUERY_STAT(BotPIEClear),false,B);Data->Test->TestFalse(TEXT("Server bot starts outside blocking geometry"),Server->OverlapBlockingTestByProfile(B->GetActorLocation(),FQuat::Identity,C->GetCollisionProfileName(),FCollisionShape::MakeCapsule(32,94),Q));}
            Next();return false;
        }
        if(Phase==1)
        {
            if(Now-At<2)return false;int32 Moved=0;
            for(auto* B:Bots)if(FVector::DistSquared(B->GetActorLocation(),Originals.FindRef(B->GetLootId()))>225)++Moved;
            Data->Test->TestTrue(TEXT("Bots autonomously patrol the real arena"),Moved>0);
            for(auto* W:Clients)
            {
                int32 Count=0,ClientMoved=0;
                for(TActorIterator<AMarau6Bot> It(W);It;++It){++Count;Data->Test->TestTrue(TEXT("Client sees same server bot identity"),Originals.Contains(It->GetLootId()));if(FVector::DistSquared(It->GetActorLocation(),Originals.FindRef(It->GetLootId()))>225)++ClientMoved;}
                Data->Test->TestEqual(TEXT("Each remote client receives three bots"),Count,3);Data->Test->TestTrue(TEXT("Each remote client sees bot patrol movement"),ClientMoved>0);
            }
            for(auto* B:Bots){B->SetActorTickEnabled(false);if(auto* AI=Cast<AAIController>(B->GetController()))AI->StopMovement();}
            for(auto* W:Clients)for(TActorIterator<AMarau6PlayerController> It(W);It;++It)if(It->IsLocalController())It->ServerEnterRaid();
            Next();return false;
        }
        AMarau6PlayerController* Controllers[2]={nullptr,nullptr};
        for(TActorIterator<AMarau6PlayerController> It(Server);It;++It)if(It->GetBaseProfile()>=1 && It->GetBaseProfile()<=2)Controllers[It->GetBaseProfile()-1]=*It;
        auto* A=Controllers[0]?Cast<AMarau6Character>(Controllers[0]->GetPawn()):nullptr;
        auto* B=Controllers[1]?Cast<AMarau6Character>(Controllers[1]->GetPawn()):nullptr;
        if(!A || !B)return false;
        if(Phase==2)
        {
            if(!Controllers[0]->IsInRaid() || !Controllers[1]->IsInRaid() || !Bridge->CanMutate())
            {
                if(Now>=NextFire){for(auto* W:Clients)for(TActorIterator<AMarau6PlayerController> It(W);It;++It)if(It->IsLocalController() && !It->IsInRaid())It->ServerEnterRaid();NextFire=Now+.5;}
                return false;
            }
            if(Bots.Num()!=3){Data->Test->AddError(TEXT("Missing bot population"));return true;}
            auto* Bot=Bots[0];bool Placed=false;
            for(int32 I=0;I<8;++I){const FVector Position=A->GetActorLocation()+FVector(FMath::Cos(I*PI/4)*240,FMath::Sin(I*PI/4)*240,0);FCollisionQueryParams Q(SCENE_QUERY_STAT(BotTestPlacement),false,Bot);if(!Server->OverlapBlockingTestByProfile(Position,FQuat::Identity,TEXT("Pawn"),FCollisionShape::MakeCapsule(34,96),Q)){Bot->SetActorLocation(Position);Placed=true;break;}}
            if(!Placed){Data->Test->AddError(TEXT("No clear combat test location"));return true;}
            Bot->GetCharacterMovement()->SetMovementMode(MOVE_Flying);B->SetActorLocation(A->GetActorLocation()+FVector(5000,0,0));B->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            Bot->SetActorRotation((A->GetActorLocation()-Bot->GetActorLocation()).Rotation());Victim=Bot;Reward=Bot->GetLootId();BeforeHealth=A->GetHealth();Bot->StepAI();Round=Bridge->GetRoundId();Next();return false;
        }
        if(Phase==3)
        {
            if(Now-At<1.2)return false;auto* Bot=Victim.Get();if(!Bot)return true;Bot->StepAI();
            Data->Test->TestEqual(TEXT("Bot fires one server accepted shot"),Bot->GetShotCount(),1);
            Data->Test->TestEqual(TEXT("Bot applies ten authoritative damage"),A->GetHealth(),BeforeHealth-10);
            Next();return false;
        }
        if(Phase==4)
        {
            if(Now<NextFire)return false;auto* Bot=Victim.Get();if(!Bot)return true;
            for(auto* W:Clients)for(TActorIterator<AMarau6PlayerController> It(W);It;++It)if(It->IsLocalController() && It->GetBaseProfile()==1)
            {
                auto* P=Cast<AMarau6Character>(It->GetPawn());if(!P)return false;
                const FRotator Aim=(Bot->GetActorLocation()-P->GetPawnViewLocation()).Rotation();It->SetControlRotation(Aim);Controllers[0]->SetControlRotation(Aim);
                P->ProcessEvent(P->FindFunction(TEXT("ServerFire")),nullptr);
            }
            ++Fired;NextFire=Now+.4;if(Fired>=4)Next();return false;
        }
        if(Phase==5)
        {
            if(Now-At<.6)return false;auto* Bot=Victim.Get();if(!Bot || Bot->IsAlive()){Data->Test->AddError(TEXT("Four client rifle shots must kill bot"));return true;}
            for(auto* W:Clients){bool Seen=false;for(TActorIterator<AMarau6Bot> It(W);It;++It)if(It->GetLootId()==Reward){Seen=true;Data->Test->TestFalse(TEXT("Bot death replicates to both clients"),It->IsAlive());Data->Test->TestEqual(TEXT("Accepted bot shot replicates"),It->GetShotCount(),1);}Data->Test->TestTrue(TEXT("Remote dead bot observed"),Seen);}
            for(auto* W:Clients)for(TActorIterator<AMarau6PlayerController> It(W);It;++It)if(It->IsLocalController() && It->GetBaseProfile()==1)if(auto* P=Cast<AMarau6Character>(It->GetPawn()))Data->Test->TestEqual(TEXT("Bot damage replicated to victim client"),P->GetHealth(),BeforeHealth-10);
            AMarau6LootActor* Loot=nullptr;int32 Count=0;for(TActorIterator<AMarau6LootActor> It(Server);It;++It)if(It->GetItem().Id==Reward){Loot=*It;++Count;}
            if(!Loot){Data->Test->AddError(TEXT("Dead bot reward absent"));return true;}Data->Test->TestEqual(TEXT("One reward world actor"),Count,1);
            for(int32 I=0;I<2;++I){auto* P=I==0?A:B;P->SetActorLocation(Loot->GetActorLocation()+FVector(-120,I==0?-80:80,96));P->GetCharacterMovement()->SetMovementMode(MOVE_Flying);Controllers[I]->SetControlRotation((Loot->GetActorLocation()-P->GetPawnViewLocation()).Rotation());}
            Next();return false;
        }
        if(Phase==6)
        {
            if(Now-At<.6)return false;
            int32 Ready=0;TArray<TPair<UMarau6InventoryComponent*,AMarau6LootActor*>> Requests;
            for(auto* W:Clients)
            {
                AMarau6LootActor* Loot=nullptr;for(TActorIterator<AMarau6LootActor> It(W);It;++It)if(It->GetItem().Id==Reward)Loot=*It;
                for(TActorIterator<AMarau6PlayerController> It(W);It;++It)if(It->IsLocalController())if(auto* P=Cast<AMarau6Character>(It->GetPawn()))if(Loot)
                {
                    It->SetControlRotation((Loot->GetActorLocation()-P->GetPawnViewLocation()).Rotation());
                    if(P->GetInventory()->LookedAtLoot()==Loot)++Ready;Requests.Add({P->GetInventory(),Loot});
                    if(Now-At>8)UE_LOG(LogTemp,Warning,TEXT("BOTS_PICKUP client profile=%d pos=%s aim=%s looked=%s reward=%s"),It->GetBaseProfile(),*P->GetActorLocation().ToString(),*It->GetControlRotation().ToString(),*GetNameSafe(P->GetInventory()->LookedAtLoot()),*Loot->GetActorLocation().ToString());
                }
            }
            for(int32 I=0;I<2;++I)
            {
                auto* P=I==0?A:B;auto* Look=P->GetInventory()->LookedAtLoot();
                if(Look && Look->GetItem().Id==Reward)++Ready;
                if(Now-At>8)UE_LOG(LogTemp,Warning,TEXT("BOTS_PICKUP server profile=%d pos=%s aim=%s looked=%s active=%d"),I+1,*P->GetActorLocation().ToString(),*Controllers[I]->GetControlRotation().ToString(),*GetNameSafe(Look),P->CanParticipate());
            }
            if(Ready!=4){if(Now-At>10){Data->Test->AddError(TEXT("Pickup views did not converge on reward"));return true;}return false;}
            for(const auto& Request:Requests)Request.Key->ServerPickup(Request.Value,Reward);
            Next();return false;
        }
        if(Phase==7)
        {
            if(Now-At<.8)return false;int32 Owners=0;
            for(int32 I=0;I<2;++I)for(const auto& Item:(I==0?A:B)->GetInventory()->GetSnapshot().Items)if(Item.Id==Reward){++Owners;Winner=I+1;}
            if(!Data->Test->TestEqual(TEXT("Competing clients have exactly one reward winner"),Owners,1))return true;
            auto* Win=Winner==1?A:B;auto* Lose=Winner==1?B:A;Lose->TakeDamage(100,FDamageEvent(),Controllers[Winner-1],Win);
            Win->SetActorLocation(FVector(1400,0,96));Win->GetCharacterMovement()->StopMovementImmediately();Next();return false;
        }
        if(Phase==8)
        {
            if(Controllers[0]->IsInRaid() || Controllers[1]->IsInRaid() || !Bridge->IsSettled())return false;
            TSharedPtr<FJsonObject> State;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Controllers[Winner-1]->GetBaseJson()),State);if(!State)return false;
            int32 Count=0;for(const auto& V:State->GetArrayField(TEXT("items")))if(V->AsObject()->GetStringField(TEXT("id"))==Reward.ToString(EGuidFormats::DigitsWithHyphensLower))++Count;
            Data->Test->TestEqual(TEXT("Extracted bot loot saved once by backend"),Count,1);
            auto Report=MakeShared<FJsonObject>();FString Run;FParse::Value(FCommandLine::Get(),TEXT("Marau6IntegrationRun="),Run);Report->SetStringField(TEXT("run"),Run);Report->SetNumberField(TEXT("profile"),Winner);Report->SetStringField(TEXT("item"),Reward.ToString(EGuidFormats::DigitsWithHyphensLower));
            FFileHelper::SaveStringToFile(UMarau6Persistence::Encode(Report),*(FPaths::ProjectDir()/TEXT("LocalSettings/bots-test-result.json")));
            for(auto* W:Clients)for(TActorIterator<AMarau6PlayerController> It(W);It;++It)if(It->IsLocalController() && It->GetBaseProfile()==Winner)It->ServerNextRound();
            Next();return false;
        }
        if(Phase==9)
        {
            if(Bridge->GetRoundId()==Round || !Bridge->CanMutate() || Bots.Num()!=3)return false;
            for(auto* W:Clients){int32 NewCount=0;for(TActorIterator<AMarau6Bot> It(W);It;++It)if(It->GetLootId().IsValid() && !Originals.Contains(It->GetLootId()))++NewCount;if(NewCount!=3)return false;}
            for(auto* Bot:Bots){Data->Test->TestFalse(TEXT("New round never reuses old bot reward IDs"),Originals.Contains(Bot->GetLootId()));Data->Test->TestTrue(TEXT("New round bots alive"),Bot->IsAlive());}
            Data->Test->TestEqual(TEXT("New round creates exactly three bots"),Server->GetSubsystem<UMarau6Bots>()->GetSpawnedCount(),3);
            Data->Test->AddInfo(TEXT("Bots PIE: patrol, replicated combat/death, contested loot, extraction persistence and round reset verified"));return true;
        }
        return false;
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6BotsPIETest,"EditorLifecycle.BotsOnlineAndTeardown",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6BotsPIETest::RunTest(const FString&)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("Marau6Integration")) || !FParse::Param(FCommandLine::Get(),TEXT("Marau6BotsTest"))){AddError(TEXT("Requires isolated integration launcher with -Bots -PIE"));return false;}
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/FirstPerson/Lvl_FirstPerson"));auto Data=MakeShared<FBotsPIEData>(this);
    Data->Settings->SetPlayNetMode(PIE_Client);Data->Settings->SetPlayNumberOfClients(2);Data->Settings->SetRunUnderOneProcess(true);
    FRequestPlaySessionParams Params;Params.EditorPlaySettings=Data->Settings.Get();Params.bAllowOnlineSubsystem=false;
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Params));ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(4));ADD_LATENT_AUTOMATION_COMMAND(FBotsPIEFlow(Data));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2));return true;
}
#endif
