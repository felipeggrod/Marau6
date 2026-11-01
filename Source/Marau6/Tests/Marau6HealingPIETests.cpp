#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "UObject/StrongObjectPtr.h"
#include "Marau6Bot.h"
#include "Marau6Character.h"
#include "Marau6PlayerController.h"
#include "Marau6HealingComponent.h"
#include "Marau6InventoryComponent.h"
#include "Marau6ItemLedger.h"
#include "Marau6Persistence.h"
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
struct FHealingPIEData
{
    FAutomationTestBase* Test;
    TStrongObjectPtr<ULevelEditorPlaySettings> Settings;
    explicit FHealingPIEData(FAutomationTestBase* T):Test(T),Settings(DuplicateObject(GetMutableDefault<ULevelEditorPlaySettings>(),GetTransientPackage())){}
};
TSharedPtr<FJsonObject> BaseItem(AMarau6PlayerController* PC,int32 Slot)
{
    TSharedPtr<FJsonObject> O;if(!PC || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(PC->GetBaseJson()),O))return nullptr;
    for(const auto& V:O->GetArrayField(TEXT("items")))if(V->AsObject()->GetIntegerField(TEXT("slot"))==Slot)return V->AsObject();return nullptr;
}
class FHealingPIEFlow : public IAutomationLatentCommand
{
    TSharedPtr<FHealingPIEData> Data;
    int32 Phase=0,Ammo=10;
    double Started=FPlatformTime::Seconds(),At=Started,NextRequest=0;
    TWeakObjectPtr<AMarau6Bot> Shooter;
    FGuid SafeKit,BagKit,OtherKit;
    void Next(){UE_LOG(LogTemp,Display,TEXT("MARAU6_HEALING_PIE phase %d passed"),Phase);++Phase;At=FPlatformTime::Seconds();}
public:
    explicit FHealingPIEFlow(TSharedPtr<FHealingPIEData> In):Data(In){}
    virtual bool Update()override
    {
        const double Now=FPlatformTime::Seconds();if(Now-Started>110){Data->Test->AddError(FString::Printf(TEXT("Healing PIE timeout phase %d"),Phase));return true;}
        UWorld* Server=nullptr;TArray<UWorld*> Clients;
        for(const auto& C:GEngine->GetWorldContexts())if(auto* W=C.World();W && W->WorldType==EWorldType::PIE){if(W->GetNetMode()==NM_DedicatedServer)Server=W;else if(W->GetNetMode()==NM_Client)Clients.Add(W);}
        if(!Server || Clients.Num()!=2)return false;
        auto* Bridge=Server->GetSubsystem<UMarau6Persistence>();
        AMarau6PlayerController* Controllers[2]={nullptr,nullptr};AMarau6Character* RemoteA=nullptr;AMarau6Character* OwnerA=nullptr;UMarau6HealingComponent* ClientHeal=nullptr;
        auto Send=[&](int32 Profile,TFunctionRef<void(AMarau6PlayerController*)> Action)
        {for(auto* W:Clients)for(TActorIterator<AMarau6PlayerController> It(W);It;++It)if(It->IsLocalController() && It->GetBaseProfile()==Profile)Action(*It);};
        if(Phase==0)
        {
            if(!Bridge->CanMutate())return false;int32 Count=0;
            for(TActorIterator<AMarau6Bot> It(Server);It;++It){++Count;It->SetActorTickEnabled(false);if(auto* AI=Cast<AAIController>(It->GetController()))AI->StopMovement();if(!Shooter.IsValid())Shooter=*It;}
            if(Count!=3)return false;int32 Profile=0;for(auto* W:Clients)for(TActorIterator<AMarau6PlayerController> It(W);It;++It)if(It->IsLocalController())It->ServerSelectProfile(++Profile);
            Next();return false;
        }
        for(TActorIterator<AMarau6PlayerController> It(Server);It;++It)if(It->GetBaseProfile()>=1 && It->GetBaseProfile()<=2)Controllers[It->GetBaseProfile()-1]=*It;
        if(!Controllers[0] || !Controllers[1])return false;
        auto* A=Cast<AMarau6Character>(Controllers[0]->GetPawn());auto* B=Cast<AMarau6Character>(Controllers[1]->GetPawn());if(!A || !B)return false;
        for(auto* W:Clients)for(TActorIterator<AMarau6PlayerController> It(W);It;++It)if(It->IsLocalController())
        {
            if(It->GetBaseProfile()==1){OwnerA=Cast<AMarau6Character>(It->GetPawn());if(OwnerA)ClientHeal=OwnerA->GetHealing();}
            else if(It->GetBaseProfile()==2)for(TActorIterator<AMarau6Character> P(W);P;++P)if(!P->IsLocallyControlled())RemoteA=*P;
        }
        if(Phase==1)
        {
            if(!BaseItem(Controllers[0],100) || !BaseItem(Controllers[1],100))return false;
            Send(1,[](auto* PC){PC->ServerBaseMove(100,8);});Send(2,[](auto* PC){PC->ServerBaseMove(100,8);});Next();return false;
        }
        if(Phase==2)
        {
            if(!BaseItem(Controllers[0],8) || !BaseItem(Controllers[1],8))return false;Send(1,[](auto* PC){PC->ServerBaseMove(101,0);});Next();return false;
        }
        if(Phase==3)
        {
            if(!BaseItem(Controllers[0],0))return false;
            FGuid::Parse(BaseItem(Controllers[0],8)->GetStringField(TEXT("id")),SafeKit);FGuid::Parse(BaseItem(Controllers[0],0)->GetStringField(TEXT("id")),BagKit);FGuid::Parse(BaseItem(Controllers[1],8)->GetStringField(TEXT("id")),OtherKit);Next();return false;
        }
        if(Phase==4)
        {
            if(!Controllers[0]->IsInRaid() || !Controllers[1]->IsInRaid() || !Bridge->CanMutate())
            {if(Now>=NextRequest){for(int32 P=1;P<=2;++P)Send(P,[](auto* PC){if(!PC->IsInRaid())PC->ServerEnterRaid();});NextRequest=Now+.5;}return false;}
            auto* Bot=Shooter.Get();if(!Bot)return true;bool Placed=false;
            for(int32 I=0;I<8;++I){const FVector Pos=A->GetActorLocation()+FVector(FMath::Cos(I*PI/4)*240,FMath::Sin(I*PI/4)*240,0);FCollisionQueryParams Q(SCENE_QUERY_STAT(HealingCombatSetup),false,Bot);if(!Server->OverlapBlockingTestByProfile(Pos,FQuat::Identity,TEXT("Pawn"),FCollisionShape::MakeCapsule(34,96),Q)){Bot->SetActorLocation(Pos);Placed=true;break;}}
            if(!Placed){Data->Test->AddError(TEXT("No clear bot damage position"));return true;}
            Bot->GetCharacterMovement()->SetMovementMode(MOVE_Flying);Bot->SetActorRotation((A->GetActorLocation()-Bot->GetActorLocation()).Rotation());B->SetActorLocation(A->GetActorLocation()+FVector(5000,0,0));B->GetCharacterMovement()->SetMovementMode(MOVE_Flying);Bot->StepAI();NextRequest=Now+1.1;Next();return false;
        }
        if(Phase==5)
        {
            auto* Bot=Shooter.Get();if(!Bot)return true;if(Now<NextRequest)return false;Bot->StepAI();NextRequest=Now+.85;if(Bot->GetShotCount()<4)return false;
            Data->Test->TestEqual(TEXT("Bot injures player to sixty HP"),A->GetHealth(),60.f);Next();return false;
        }
        if(!ClientHeal || !OwnerA || !RemoteA)return false;
        if(Phase==6){ClientHeal->ServerUse(SafeKit);Next();return false;}
        if(Phase==7)
        {
            if(!A->GetHealing()->IsHealing() || !ClientHeal->IsHealing())return false;if(Now<NextRequest)return false;
            Shooter->StepAI();Data->Test->TestFalse(TEXT("Bot damage interrupts active treatment"),A->GetHealing()->IsHealing());Data->Test->TestEqual(TEXT("Interrupted player has fifty HP"),A->GetHealth(),50.f);
            Data->Test->TestTrue(TEXT("Interrupted safe kit not consumed"),A->GetInventory()->ItemAt(8) && A->GetInventory()->ItemAt(8)->Id==SafeKit);Next();return false;
        }
        if(Phase==8)
        {
            if(ClientHeal->IsHealing() || OwnerA->GetHealth()!=50)return false;ClientHeal->ServerUse(SafeKit);Ammo=A->GetMagazineAmmo();Next();return false;
        }
        if(Phase==9)
        {
            if(!ClientHeal->IsHealing() || Now-At<2)return false;
            Data->Test->TestEqual(TEXT("No healing before four seconds"),A->GetHealth(),50.f);Data->Test->TestEqual(TEXT("Observer sees current injured health"),RemoteA->GetHealth(),50.f);
            OwnerA->ProcessEvent(OwnerA->FindFunction(TEXT("ServerFire")),nullptr);OwnerA->ProcessEvent(OwnerA->FindFunction(TEXT("ServerReload")),nullptr);Next();return false;
        }
        if(Phase==10)
        {
            if(A->GetHealing()->IsHealing() || ClientHeal->IsHealing() || OwnerA->GetHealth()!=90 || RemoteA->GetHealth()!=90 || OwnerA->GetInventory()->ItemAt(8))return false;
            Data->Test->TestEqual(TEXT("Network fire blocked while healing"),A->GetMagazineAmmo(),Ammo);Data->Test->TestEqual(TEXT("Forty HP effect replicated to observer"),A->GetHealth(),90.f);Data->Test->TestNull(TEXT("Safe source removed on server"),A->GetInventory()->ItemAt(8));Data->Test->TestNull(TEXT("Safe source removed on owning client"),OwnerA->GetInventory()->ItemAt(8));
            ClientHeal->ServerUse(SafeKit);Next();return false;
        }
        if(Phase==11)
        {
            if(Now-At<.4)return false;Data->Test->TestFalse(TEXT("Consumed GUID reuse rejected"),A->GetHealing()->IsHealing());Data->Test->TestEqual(TEXT("No repeat healing"),A->GetHealth(),90.f);ClientHeal->ServerUse(OtherKit);Next();return false;
        }
        if(Phase==12)
        {
            if(Now-At<.4)return false;Data->Test->TestFalse(TEXT("Other player's kit request rejected"),A->GetHealing()->IsHealing());ClientHeal->ServerUse(BagKit);Next();return false;
        }
        if(Phase==13)
        {
            if(Now-At<4.3 || ClientHeal->IsHealing() || OwnerA->GetHealth()!=100 || RemoteA->GetHealth()!=100)return false;
            Data->Test->TestEqual(TEXT("Second kit clamps health to100"),A->GetHealth(),100.f);Data->Test->TestNull(TEXT("Bag source also consumed once"),A->GetInventory()->ItemAt(0));
            auto* L=Server->GetSubsystem<UMarau6ItemLedger>();int32 Consumed=0;for(const auto& V:L->Export())if(V->AsObject()->GetStringField(TEXT("custody"))==TEXT("consumed"))++Consumed;Data->Test->TestEqual(TEXT("Exactly two terminal consumption records"),Consumed,2);
            B->TakeDamage(100,FDamageEvent(),Controllers[0],A);A->SetActorLocation(FVector(1400,0,96));A->GetCharacterMovement()->StopMovementImmediately();Next();return false;
        }
        if(Phase==14)
        {
            if(Controllers[0]->IsInRaid() || Controllers[1]->IsInRaid() || !Bridge->IsSettled())return false;
            Data->Test->TestFalse(TEXT("Extraction never returns consumed safe GUID"),Controllers[0]->GetBaseJson().Contains(SafeKit.ToString(EGuidFormats::DigitsWithHyphensLower)));
            Data->Test->TestFalse(TEXT("Extraction never returns consumed bag GUID"),Controllers[0]->GetBaseJson().Contains(BagKit.ToString(EGuidFormats::DigitsWithHyphensLower)));
            Data->Test->TestTrue(TEXT("Other player's unused safe kit survives death"),Controllers[1]->GetBaseJson().Contains(OtherKit.ToString(EGuidFormats::DigitsWithHyphensLower)));
            auto Report=MakeShared<FJsonObject>();FString Run;FParse::Value(FCommandLine::Get(),TEXT("Marau6IntegrationRun="),Run);Report->SetStringField(TEXT("run"),Run);Report->SetStringField(TEXT("raid"),Bridge->GetRoundId());Report->SetStringField(TEXT("safe"),SafeKit.ToString(EGuidFormats::DigitsWithHyphensLower));Report->SetStringField(TEXT("bag"),BagKit.ToString(EGuidFormats::DigitsWithHyphensLower));Report->SetStringField(TEXT("kept"),OtherKit.ToString(EGuidFormats::DigitsWithHyphensLower));FFileHelper::SaveStringToFile(UMarau6Persistence::Encode(Report),*(FPaths::ProjectDir()/TEXT("LocalSettings/healing-test-result.json")));
            Data->Test->AddInfo(TEXT("Healing PIE: bot damage, interruption, delayed safe/bag use, observer health, replay/foreign UUID rejection and durable outcomes verified"));return true;
        }
        return false;
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6HealingPIETest,"EditorLifecycle.HealingOnlineAndTeardown",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6HealingPIETest::RunTest(const FString&)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("Marau6Integration")) || !FParse::Param(FCommandLine::Get(),TEXT("Marau6HealingTest"))){AddError(TEXT("Requires isolated integration launcher -PIE -Healing"));return false;}
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/FirstPerson/Lvl_FirstPerson"));auto Data=MakeShared<FHealingPIEData>(this);Data->Settings->SetPlayNetMode(PIE_Client);Data->Settings->SetPlayNumberOfClients(2);Data->Settings->SetRunUnderOneProcess(true);FRequestPlaySessionParams Params;Params.EditorPlaySettings=Data->Settings.Get();Params.bAllowOnlineSubsystem=false;
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Params));ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(4));ADD_LATENT_AUTOMATION_COMMAND(FHealingPIEFlow(Data));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2));return true;
}
#endif
