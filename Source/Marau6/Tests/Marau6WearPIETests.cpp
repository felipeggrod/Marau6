#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "UObject/StrongObjectPtr.h"
#include "Marau6Character.h"
#include "Marau6PlayerController.h"
#include "Marau6InventoryComponent.h"
#include "Marau6ItemLedger.h"
#include "Marau6Persistence.h"
#include "Marau6LootActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
namespace
{
struct FWearPIEData
{
    FAutomationTestBase* Test;
    TStrongObjectPtr<ULevelEditorPlaySettings> Settings;
    explicit FWearPIEData(FAutomationTestBase* T):Test(T),Settings(DuplicateObject(GetMutableDefault<ULevelEditorPlaySettings>(),GetTransientPackage())){}
};
class FWearPIEFlow : public IAutomationLatentCommand
{
    TSharedPtr<FWearPIEData> D;
    int32 Phase=0,Sent=0;
    double Started=FPlatformTime::Seconds(),At=Started,NextRequest=0;
    FGuid Rifle;
    FString Round;
    void Next(){D->Test->AddInfo(FString::Printf(TEXT("Wear PIE phase %d"),Phase));++Phase;At=FPlatformTime::Seconds();NextRequest=0;}
    void Request(AMarau6Character* P,const TCHAR* Name){P->ProcessEvent(P->FindFunction(Name),nullptr);}
public:
    explicit FWearPIEFlow(TSharedPtr<FWearPIEData> In):D(In){}
    bool Update() override
    {
        const double Now=FPlatformTime::Seconds();
        if(Now-Started>115){D->Test->AddError(FString::Printf(TEXT("Wear PIE timeout phase %d sent %d"),Phase,Sent));return true;}
        UWorld* Server=nullptr;TArray<UWorld*> Clients;
        for(const auto& C:GEngine->GetWorldContexts())if(auto* W=C.World();W && W->WorldType==EWorldType::PIE)
        {if(W->GetNetMode()==NM_DedicatedServer)Server=W;else if(W->GetNetMode()==NM_Client)Clients.Add(W);}
        if(!Server || Clients.Num()!=2)return false;
        auto* Bridge=Server->GetSubsystem<UMarau6Persistence>();
        AMarau6PlayerController* Local[2]={nullptr,nullptr};AMarau6PlayerController* Remote[2]={nullptr,nullptr};
        for(int32 n=0;n<2;n++)for(TActorIterator<AMarau6PlayerController> It(Clients[n]);It;++It)if(It->IsLocalController())Local[n]=*It;
        for(TActorIterator<AMarau6PlayerController> It(Server);It;++It)if(It->GetBaseProfile()>=1 && It->GetBaseProfile()<=2)Remote[It->GetBaseProfile()-1]=*It;
        if(!Local[0] || !Local[1])return false;
        if(Phase==0){if(!Bridge->CanMutate())return false;Local[0]->ServerSelectProfile(1);Local[1]->ServerSelectProfile(2);Next();return false;}
        if(Phase==1)
        {
            if(Local[0]->GetBaseProfile()!=1 || Local[1]->GetBaseProfile()!=2 || Local[0]->GetBaseJson().IsEmpty() || Local[1]->GetBaseJson().IsEmpty())return false;
            Local[0]->ServerEnterRaid();Local[1]->ServerEnterRaid();Next();return false;
        }
        if(!Remote[0] || !Remote[1])return false;
        auto* A=Cast<AMarau6Character>(Remote[0]->GetPawn());auto* B=Cast<AMarau6Character>(Remote[1]->GetPawn());
        auto* CA=Cast<AMarau6Character>(Local[0]->GetPawn());auto* CB=Cast<AMarau6Character>(Local[1]->GetPawn());
        if(!A || !B || !CA || !CB)return false;
        if(Phase==2)
        {
            if(Now>=NextRequest){for(auto* PC:Local)if(!PC->IsInRaid())PC->ServerEnterRaid();NextRequest=Now+.5;}
            if(!CA->CanParticipate() || !CB->CanParticipate() || !CA->GetInventory()->ItemAt(10) || !CB->GetInventory()->ItemAt(10) || !Bridge->IsSettled())return false;
            Rifle=A->GetInventory()->ItemAt(10)->Id;Round=Bridge->GetRoundId();
            D->Test->TestEqual(TEXT("Fresh network rifle reserve40"),CA->GetReserveAmmo(),40);D->Test->TestEqual(TEXT("Initial network condition"),CA->GetRifleDurability(),10000);
            Local[0]->SetControlRotation(FRotator(75,90,0));Remote[0]->SetControlRotation(FRotator(75,90,0));Next();return false;
        }
        if(Phase==3)
        {
            const int32 Accepted=(10000-A->GetRifleDurability())/250;
            if(Accepted==40){Next();return false;}
            if(Now<NextRequest)return false;
            NextRequest=Now+.4;
            if(A->GetMagazineAmmo()==0){Request(CA,TEXT("ServerReload"));return false;}
            if(A->IsReloading())return false;
            if(Accepted<Sent)return false;
            D->Test->TestEqual(TEXT("Owning client receives each accepted wear change"),CA->GetRifleDurability(),A->GetRifleDurability());
            Request(CA,TEXT("ServerFire"));Sent++;return false;
        }
        if(Phase==4)
        {
            if(CA->GetRifleDurability()!=0)return false;
            if(A->GetMagazineAmmo()==0){if(Now>=NextRequest){Request(CA,TEXT("ServerReload"));NextRequest=Now+.4;}return false;}
            if(CA->GetMagazineAmmo()!=10 || A->IsReloading())return false;
            Request(CA,TEXT("ServerFire"));Next();return false;
        }
        if(Phase==5)
        {
            if(Now-At<.7)return false;
            D->Test->TestEqual(TEXT("Server rejects shot41 with loaded ammunition"),A->GetMagazineAmmo(),10);D->Test->TestEqual(TEXT("Owner retains zero"),CA->GetRifleDurability(),0);
            CA->GetInventory()->ServerDrop(Rifle);Next();return false;
        }
        if(Phase==6)
        {
            if(B->GetInventory()->ItemAt(0) && B->GetInventory()->ItemAt(0)->Id==Rifle){Next();return false;}
            AMarau6LootActor* Ground=nullptr;for(TActorIterator<AMarau6LootActor> It(Server);It;++It)if(It->GetItem().Id==Rifle)Ground=*It;
            if(!Ground)return false;
            D->Test->TestEqual(TEXT("Dropped actor carries zero"),Ground->GetItem().Durability,0);
            B->SetActorLocation(Ground->GetActorLocation()+FVector(-100,0,96));B->GetCharacterMovement()->StopMovementImmediately();
            const FRotator Aim=(Ground->GetActorLocation()-B->GetPawnViewLocation()).Rotation();Local[1]->SetControlRotation(Aim);Remote[1]->SetControlRotation(Aim);
            if(Now-At<.7)return false;
            for(TActorIterator<AMarau6LootActor> It(Clients[1]);It;++It)if(It->GetItem().Id==Rifle && Now>=NextRequest){CB->GetInventory()->ServerPickup(*It,Rifle);NextRequest=Now+.4;}
            if(B->GetInventory()->ItemAt(0) && B->GetInventory()->ItemAt(0)->Id==Rifle){Next();return false;}
            return false;
        }
        if(Phase==7)
        {
            if(B->GetInventory()->ItemAt(10)->Id!=Rifle){if(Now>=NextRequest){CB->GetInventory()->ServerMove(Rifle,10);NextRequest=Now+.4;}return false;}
            if(CB->GetRifleDurability()!=0)return false;
            Request(CB,TEXT("ServerFire"));Next();return false;
        }
        if(Phase==8)
        {
            if(Now-At<.6)return false;D->Test->TestEqual(TEXT("New owner cannot fire broken transferred rifle"),B->GetMagazineAmmo(),10);
            A->TakeDamage(100,FDamageEvent(),Remote[1],B);B->SetActorLocation(FVector(1400,0,96));B->GetCharacterMovement()->StopMovementImmediately();Next();return false;
        }
        if(Phase==9)
        {
            if(Remote[0]->IsInRaid() || Remote[1]->IsInRaid() || !Bridge->IsSettled())return false;
            TSharedPtr<FJsonObject> State;if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Local[1]->GetBaseJson()),State))return false;
            int32 Found=0;for(const auto& V:State->GetArrayField(TEXT("items")))if(V->AsObject()->GetStringField(TEXT("id"))==Rifle.ToString(EGuidFormats::DigitsWithHyphensLower))
            {++Found;D->Test->TestEqual(TEXT("Extracted base condition remains zero"),V->AsObject()->GetIntegerField(TEXT("durability")),0);}
            if(Found!=1)return false;
            auto Report=MakeShared<FJsonObject>();FString Run;FParse::Value(FCommandLine::Get(),TEXT("Marau6IntegrationRun="),Run);Report->SetStringField(TEXT("run"),Run);Report->SetStringField(TEXT("raid"),Round);Report->SetStringField(TEXT("item"),Rifle.ToString(EGuidFormats::DigitsWithHyphensLower));
            FFileHelper::SaveStringToFile(UMarau6Persistence::Encode(Report),*(FPaths::ProjectDir()/TEXT("LocalSettings/wear-test-result.json")));
            Local[1]->ServerNextRound();Next();return false;
        }
        if(Phase==10)
        {
            if(Bridge->GetRoundId()==Round || !Bridge->CanMutate())return false;
            Local[1]->ServerEnterRaid();Next();return false;
        }
        if(Phase==11)
        {
            if(!CB->CanParticipate() || !CB->GetInventory()->ItemAt(10) || CB->GetInventory()->ItemAt(10)->Id!=Rifle)return false;
            D->Test->TestEqual(TEXT("Next raid retains broken condition"),CB->GetRifleDurability(),0);Request(CB,TEXT("ServerFire"));Next();return false;
        }
        if(Phase==12)
        {
            if(Now-At<.6)return false;D->Test->TestEqual(TEXT("Next raid still blocks fire without spending ammo"),B->GetMagazineAmmo(),10);
            B->SetActorLocation(FVector(1400,0,96));B->GetCharacterMovement()->StopMovementImmediately();Next();return false;
        }
        if(Phase==13)return !Remote[1]->IsInRaid() && Bridge->IsSettled();
        return false;
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6WearPIETest,"EditorLifecycle.WearOnlineAndTeardown",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6WearPIETest::RunTest(const FString&)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("Marau6Integration")) || !FParse::Param(FCommandLine::Get(),TEXT("Marau6WearTest"))){AddError(TEXT("Requires isolated launcher -PIE -Wear"));return false;}
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/FirstPerson/Lvl_FirstPerson"));auto D=MakeShared<FWearPIEData>(this);
    D->Settings->SetPlayNetMode(PIE_Client);D->Settings->SetPlayNumberOfClients(2);D->Settings->SetRunUnderOneProcess(true);
    FRequestPlaySessionParams P;P.EditorPlaySettings=D->Settings.Get();P.bAllowOnlineSubsystem=false;
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(P));ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(4));ADD_LATENT_AUTOMATION_COMMAND(FWearPIEFlow(D));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2));return true;
}
#endif
