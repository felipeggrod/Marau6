#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Marau6PlayerController.h"
#include "Marau6Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Marau6Persistence.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
struct FPIERegression
{
    FAutomationTestBase* Test;
    bool bOnline=false;
    TStrongObjectPtr<ULevelEditorPlaySettings> Settings;
    explicit FPIERegression(FAutomationTestBase* InTest):Test(InTest),Settings(DuplicateObject(GetMutableDefault<ULevelEditorPlaySettings>(),GetTransientPackage())){}
};
class FPIEClientsCheck : public IAutomationLatentCommand
{
    TSharedPtr<FPIERegression> Data;bool bSend;
public:
    FPIEClientsCheck(TSharedPtr<FPIERegression> In,bool Send):Data(In),bSend(Send){}
    virtual bool Update() override
    {
        int32 Clients=0,Servers=0;
        for(const auto& Context:GEngine->GetWorldContexts())
        {
            UWorld* W=Context.World();if(!W || W->WorldType!=EWorldType::PIE)continue;
            if(W->GetNetMode()==NM_DedicatedServer)++Servers;
            if(W->GetNetMode()!=NM_Client)continue;
            ++Clients;
            for(TActorIterator<AMarau6PlayerController> It(W);It;++It)if(It->IsLocalController())
            {
                if(bSend)It->ServerSelectProfile(Clients);
                else
                {Data->Test->TestTrue(TEXT("Remote client shows the base"),It->IsUsingBase());if(Data->bOnline){Data->Test->TestFalse(TEXT("Remote client receives persistent profile items"),It->GetBaseJson().IsEmpty());Data->Test->TestTrue(TEXT("Remote profile selected"),It->GetBaseProfile()>0);}else Data->Test->TestTrue(TEXT("Remote client explains unavailable backend"),It->GetBaseMessage().Contains(TEXT("backend")));Data->Test->TestFalse(TEXT("Remote client cannot enter without backend"),It->IsInRaid());}
            }
        }
        Data->Test->TestEqual(TEXT("Two real PIE client worlds"),Clients,2);
        Data->Test->TestEqual(TEXT("One dedicated PIE server"),Servers,1);
        return true;
    }
};
class FPIEEnterCheck : public IAutomationLatentCommand
{
    TSharedPtr<FPIERegression> Data;double Started=FPlatformTime::Seconds(),LastSent=0;
public:
    explicit FPIEEnterCheck(TSharedPtr<FPIERegression> In):Data(In){}
    virtual bool Update() override
    {
        const double Now=FPlatformTime::Seconds();int32 Entered=0;
        for(const auto& C:GEngine->GetWorldContexts())
        {
            UWorld* W=C.World();if(!W || W->WorldType!=EWorldType::PIE || W->GetNetMode()!=NM_Client)continue;
            for(TActorIterator<AMarau6PlayerController> It(W);It;++It)if(It->IsLocalController())
            {
                if(!It->IsInRaid()){if(Now-LastSent>.25)It->ServerEnterRaid();continue;}
                if(auto* P=Cast<AMarau6Character>(It->GetPawn());P && P->CanParticipate() && P->HasEquippedRifle())
                {++Entered;Data->Test->TestEqual(TEXT("Reserved rifle ammo replicated to remote client"),P->GetMagazineAmmo(),10);}
            }
        }
        if(Now-LastSent>.25)LastSent=Now;
        if(Entered==2)return true;
        if(Now-Started>20){Data->Test->AddError(TEXT("Both remote clients must enter with reserved equipment"));return true;}
        return false;
    }
};
class FPIESpawnSafetyCheck : public IAutomationLatentCommand
{
    TSharedPtr<FPIERegression> Data;
public:
    explicit FPIESpawnSafetyCheck(TSharedPtr<FPIERegression> In):Data(In){}
    virtual bool Update() override
    {
        for(const auto& Context:GEngine->GetWorldContexts())
        {
            UWorld* W=Context.World();if(!W || W->WorldType!=EWorldType::PIE)continue;
            if(W->GetNetMode()==NM_DedicatedServer)for(TActorIterator<APlayerStart> It(W);It;++It)Data->Test->AddInfo(FString::Printf(TEXT("PlayerStart %s"),*It->GetActorLocation().ToString()));
            for(TActorIterator<AMarau6Character> It(W);It;++It)
            {
                if(It->IsAtBase() || (W->GetNetMode()==NM_Client && !It->IsLocallyControlled()))continue;
                auto* Capsule=It->GetCapsuleComponent();
                const FCollisionShape Shape=FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius()-2.f,Capsule->GetScaledCapsuleHalfHeight()-2.f);
                FCollisionQueryParams Q(SCENE_QUERY_STAT(Marau6SpawnSafety),false,*It);
                const bool Blocked=W->OverlapBlockingTestByProfile(It->GetActorLocation(),FQuat::Identity,Capsule->GetCollisionProfileName(),Shape,Q);
                Data->Test->AddInfo(FString::Printf(TEXT("Entry net=%d location=%s movement=%d blocked=%d"),static_cast<int32>(W->GetNetMode()),*It->GetActorLocation().ToString(),static_cast<int32>(It->GetCharacterMovement()->MovementMode),Blocked));
                Data->Test->TestFalse(TEXT("Entry capsule is clear of terrain and other players"),Blocked);
                Data->Test->TestTrue(TEXT("Entry restores movement on server and owning client"),It->GetCharacterMovement()->MovementMode!=MOVE_None);
            }
        }
        return true;
    }
};
class FPIEMovementCheck : public IAutomationLatentCommand
{
    TSharedPtr<FPIERegression> Data;double Started=0;
    TMap<TWeakObjectPtr<AMarau6Character>,FVector> Origins,Directions;
public:
    explicit FPIEMovementCheck(TSharedPtr<FPIERegression> In):Data(In){}
    virtual bool Update() override
    {
        const double Now=FPlatformTime::Seconds();
        if(Started==0)
        {
            Started=Now;
            for(const auto& C:GEngine->GetWorldContexts())
            {
                UWorld* W=C.World();if(!W || W->WorldType!=EWorldType::PIE)continue;
                for(TActorIterator<AMarau6Character> It(W);It;++It)
                    if(!It->IsAtBase() && (W->GetNetMode()!=NM_Client || It->IsLocallyControlled()))Origins.Add(*It,It->GetActorLocation());
            }
        }
        if(Now-Started<1.0)
        {
            for(const auto& Pair:Origins)
            {
                auto* P=Pair.Key.Get();if(!P || !P->IsLocallyControlled() || P->GetWorld()->GetNetMode()!=NM_Client)continue;
                if(!Directions.Contains(P))
                {
                    FVector Direction(1,0,0);
                    const FVector Choices[]={FVector(1,0,0),FVector(-1,0,0),FVector(0,1,0),FVector(0,-1,0)};
                    auto* Capsule=P->GetCapsuleComponent();
                    const auto Shape=FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius()-2.f,Capsule->GetScaledCapsuleHalfHeight()-2.f);
                    FCollisionQueryParams Q(SCENE_QUERY_STAT(Marau6WalkTest),false,P);
                    for(const auto& D:Choices)if(!P->GetWorld()->SweepTestByProfile(P->GetActorLocation(),P->GetActorLocation()+D*140.f,FQuat::Identity,Capsule->GetCollisionProfileName(),Shape,Q)){Direction=D;break;}
                    Directions.Add(P,Direction);
                }
                const FVector D=Directions.FindChecked(P);
                struct FMoveParams{float Right,Forward;} Move{static_cast<float>(FVector::DotProduct(D,P->GetActorRightVector())),static_cast<float>(FVector::DotProduct(D,P->GetActorForwardVector()))};
                P->ProcessEvent(P->FindFunctionChecked(TEXT("DoMove")),&Move);
            }
            return false;
        }
        for(const auto& Pair:Origins)if(auto* P=Pair.Key.Get())
        {
            const double Distance=FVector::Dist2D(Pair.Value,P->GetActorLocation());
            Data->Test->AddInfo(FString::Printf(TEXT("Movement net=%d distance=%.1f"),static_cast<int32>(P->GetWorld()->GetNetMode()),Distance));
            Data->Test->TestTrue(TEXT("Client movement displaces pawn and reaches server"),Distance>15.f);
        }
        Data->Test->TestEqual(TEXT("Movement observed on two clients and two server pawns"),Origins.Num(),4);
        return true;
    }
};
class FPIEEndedCheck : public IAutomationLatentCommand
{
    TSharedPtr<FPIERegression> Data;
public:
    explicit FPIEEndedCheck(TSharedPtr<FPIERegression> In):Data(In){}
    virtual bool Update() override
    {
        int32 Remaining=0;for(const auto& C:GEngine->GetWorldContexts())if(C.World() && C.World()->WorldType==EWorldType::PIE)++Remaining;
        Data->Test->TestEqual(TEXT("End Play removes every PIE world without crash"),Remaining,0);return true;
    }
};
}
// Separate root: uses an isolated integration configuration on5187.
namespace
{
bool StartPIERegression(FAutomationTestBase* Test,bool Online)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("Marau6Integration"))){Test->AddError(TEXT("Use -Marau6Integration with a unique -Marau6IntegrationRun"));return false;}
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/FirstPerson/Lvl_FirstPerson"));
    auto Data=MakeShared<FPIERegression>(Test);Data->bOnline=Online;
    Data->Settings->SetPlayNetMode(PIE_Client);Data->Settings->SetPlayNumberOfClients(2);Data->Settings->SetRunUnderOneProcess(true);
    FRequestPlaySessionParams Params;Params.EditorPlaySettings=Data->Settings.Get();Params.bAllowOnlineSubsystem=false;
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Params));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(4.f));
    ADD_LATENT_AUTOMATION_COMMAND(FPIEClientsCheck(Data,true));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
    ADD_LATENT_AUTOMATION_COMMAND(FPIEClientsCheck(Data,false));
    if(Online){ADD_LATENT_AUTOMATION_COMMAND(FPIEEnterCheck(Data));ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));ADD_LATENT_AUTOMATION_COMMAND(FPIESpawnSafetyCheck(Data));ADD_LATENT_AUTOMATION_COMMAND(FPIEMovementCheck(Data));}
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
    ADD_LATENT_AUTOMATION_COMMAND(FPIEEndedCheck(Data));
    return true;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6PIETeardownTest,"EditorLifecycle.BaseOfflineAndTeardown",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6PIETeardownTest::RunTest(const FString& Parameters){return StartPIERegression(this,false);}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6PIEOnlineTest,"EditorLifecycle.BaseOnlineAndTeardown",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6PIEOnlineTest::RunTest(const FString& Parameters){return StartPIERegression(this,true);}
#endif
