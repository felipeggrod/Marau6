#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "UObject/StrongObjectPtr.h"
#include "Marau6DestructibleSection.h"
#include "Marau6Destruction.h"
#include "Marau6Character.h"
#include "Marau6PlayerController.h"
#include "Marau6Persistence.h"
#include "Marau6Bot.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NavMesh/RecastNavMesh.h"
#include "AIController.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Editor.h"
namespace
{
struct FSectionPIEData
{
    FAutomationTestBase* Test;TStrongObjectPtr<ULevelEditorPlaySettings> Settings;
    explicit FSectionPIEData(FAutomationTestBase* T):Test(T),Settings(DuplicateObject(GetMutableDefault<ULevelEditorPlaySettings>(),GetTransientPackage())){}
};
AMarau6DestructibleSection* Panel(UWorld* W,int32 Group,int32 Index)
{for(TActorIterator<AMarau6DestructibleSection> It(W);It;++It)if(It->GetBarrierIndex()==Group && It->GetSectionIndex()==Index)return *It;return nullptr;}
class FSectionPIEFlow : public IAutomationLatentCommand
{
    TSharedPtr<FSectionPIEData> D;int32 Phase=0,Sent=0;double Started=FPlatformTime::Seconds(),At=Started,NextShot=0;FString Round;
    FVector Left,Right,NavLeft,NavRight,PreviousPlayer,PreviousBot;float DetourLength=0;bool Render=false,PlayerCrossed=false,BotCrossed=false;TWeakObjectPtr<AMarau6Bot> TestBot;
    bool CrossesOpening(const FVector& Before,const FVector& After,const FVector& Center)
    {
        const float Delta=After.X-Before.X;if(FMath::Abs(Delta)<KINDA_SMALL_NUMBER)return false;
        const float Alpha=(Center.X-Before.X)/Delta;
        return Alpha>=0 && Alpha<=1 && FMath::Abs(FMath::Lerp(Before.Y,After.Y,Alpha)-Center.Y)<40;
    }
    void Next(){UE_LOG(LogTemp,Display,TEXT("MARAU6_DESTRUCTION phase=%d passed"),Phase);++Phase;At=FPlatformTime::Seconds();}
    void Capture(AMarau6PlayerController* PC,const TCHAR* Name)
    {
        if(!Render || !PC || !PC->GetLocalPlayer())return;auto* V=PC->GetLocalPlayer()->ViewportClient->Viewport;if(!V)return;
        TArray<FColor> Pixels;if(V->ReadPixels(Pixels)){TArray<uint8> PNG;const auto Size=V->GetSizeXY();FImageUtils::CompressImageArray(Size.X,Size.Y,Pixels,PNG);FFileHelper::SaveArrayToFile(PNG,*(FPaths::ProjectDir()/TEXT("LocalSettings")/Name));}
    }
public:
    explicit FSectionPIEFlow(TSharedPtr<FSectionPIEData> In):D(In){Render=FParse::Param(FCommandLine::Get(),TEXT("Marau6DestructionRender"));}
    bool Update() override
    {
        const double Now=FPlatformTime::Seconds();if(Now-Started>150){D->Test->AddError(FString::Printf(TEXT("Destruction timeout phase%d"),Phase));return true;}
        UWorld* Server=nullptr;TArray<UWorld*> Clients;for(const auto& C:GEngine->GetWorldContexts())if(auto* W=C.World();W && W->WorldType==EWorldType::PIE){if(W->GetNetMode()==NM_DedicatedServer)Server=W;else if(W->GetNetMode()==NM_Client)Clients.Add(W);}
        if(!Server || Clients.Num()<2)return false;
        AMarau6PlayerController* Local[2]={Clients[0]->GetFirstPlayerController<AMarau6PlayerController>(),Clients[1]->GetFirstPlayerController<AMarau6PlayerController>()};
        AMarau6PlayerController* Remote[2]={nullptr,nullptr};for(TActorIterator<AMarau6PlayerController> It(Server);It;++It)if(It->GetBaseProfile()>=1 && It->GetBaseProfile()<=2)Remote[It->GetBaseProfile()-1]=*It;
        auto* Bridge=Server->GetSubsystem<UMarau6Persistence>();if(!Local[0] || !Local[1] || !Bridge)return false;
        if(Phase==0)
        {
            if(!Bridge->CanMutate() || Server->GetSubsystem<UMarau6Destruction>()->GetSectionCount()!=9)return false;
            auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(Server);auto* Data=Nav?Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate):nullptr;
            D->Test->TestTrue(TEXT("Server navigation supports dynamic geometry"),Data && Data->GetRuntimeGenerationMode()==ERuntimeGenerationType::Dynamic);
            Local[0]->ServerSelectProfile(1);Local[1]->ServerSelectProfile(2);Next();return false;
        }
        if(Phase==1){if(Local[0]->GetBaseJson().IsEmpty() || Local[1]->GetBaseJson().IsEmpty())return false;Local[0]->ServerEnterRaid();Local[1]->ServerEnterRaid();Next();return false;}
        if(!Remote[0] || !Remote[1])return false;
        auto* A=Cast<AMarau6Character>(Remote[0]->GetPawn());auto* B=Cast<AMarau6Character>(Remote[1]->GetPawn());auto* CA=Cast<AMarau6Character>(Local[0]->GetPawn());
        auto* P=Panel(Server,0,1);if(!A || !B || !CA || !P)return false;
        if(Phase==2)
        {
            if(Now>=NextShot){for(auto* PC:Local)if(!PC->IsInRaid())PC->ServerEnterRaid();NextShot=Now+.5;}
            if(!A->CanParticipate() || !B->CanParticipate() || !Panel(Clients[0],0,1) || !Panel(Clients[1],0,1) || Now-At<2)return false;
            auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(Server);if(Nav->IsNavigationBuildInProgress())return false;
            Left=P->GetActorLocation()+FVector(-220,0,-26);Right=P->GetActorLocation()+FVector(220,0,-26);Round=Bridge->GetRoundId();
            FNavLocation StartNav,EndNav;
            if(!Nav->ProjectPointToNavigation(Left,StartNav,FVector(50,50,150)) || !Nav->ProjectPointToNavigation(Right,EndNav,FVector(50,50,150))){D->Test->AddError(TEXT("Barrier approaches are not navigable"));return true;}
            NavLeft=StartNav.Location;NavRight=EndNav.Location;
            auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(Server,NavLeft,NavRight);DetourLength=Path && Path->IsValid()?Path->GetPathLength():0;
            UE_LOG(LogTemp,Display,TEXT("MARAU6_DESTRUCTION navigation left=%s right=%s length=%.1f"),*NavLeft.ToString(),*NavRight.ToString(),DetourLength);
            D->Test->TestTrue(TEXT("Intact panels force navigation detour"),DetourLength>FVector::Dist2D(Left,Right)+60);
            A->SetActorLocation(Left);B->SetActorLocation(Right);A->GetCharacterMovement()->StopMovementImmediately();B->GetCharacterMovement()->StopMovementImmediately();
            Remote[0]->ClientSetLocation(Left,FRotator::ZeroRotator);Remote[1]->ClientSetLocation(Right,FRotator(0,180,0));Next();return false;
        }
        if(Phase==3 || Phase==4)
        {
            if(Now-At<.7)return false;
            const int32 Goal=Phase==3?4:8;
            const auto Aim=(P->GetActorLocation()-A->GetPawnViewLocation()).Rotation();Local[0]->SetControlRotation(Aim);Remote[0]->SetControlRotation(Aim);
            if(Sent==0 && Now>=NextShot)Capture(Local[0],TEXT("destruction-intact.png"));
            if(A->GetMagazineAmmo()!=20-Sent || Panel(Clients[0],0,1)->GetHealth()!=P->GetHealth() || Panel(Clients[1],0,1)->GetHealth()!=P->GetHealth())return false;
            if(Sent<Goal){if(Now<NextShot)return false;CA->ProcessEvent(CA->FindFunction(TEXT("ServerFire")),nullptr);++Sent;NextShot=Now+.4;return false;}
            if(Phase==3){D->Test->TestEqual(TEXT("Four shots damage one section"),P->GetHealth(),100.f);Capture(Local[0],TEXT("destruction-damaged.png"));}
            else{D->Test->TestTrue(TEXT("Eight shots break panel on both clients"),P->IsBroken() && Panel(Clients[0],0,1)->IsBroken() && Panel(Clients[1],0,1)->IsBroken());D->Test->TestEqual(TEXT("Neighbor unchanged"),Panel(Server,0,0)->GetHealth(),200.f);D->Test->TestEqual(TEXT("Breaking shot cannot damage behind wall"),B->GetHealth(),100.f);}
            Next();return false;
        }
        if(Phase==5)
        {
            if(Now-At<1)return false;Capture(Local[0],TEXT("destruction-broken.png"));
            D->Test->TestFalse(TEXT("Client collision removed"),Cast<UPrimitiveComponent>(Panel(Clients[1],0,1)->GetRootComponent())->IsCollisionEnabled());GEditor->RequestLateJoin();Next();return false;
        }
        if(Phase==6)
        {
            if(Clients.Num()<3 || !Panel(Clients.Last(),0,1))return false;
            int32 LateEffects=0;for(TActorIterator<AMarau6SectionDebris> It(Clients.Last());It;++It)++LateEffects;D->Test->TestEqual(TEXT("Late state does not replay old break effects"),LateEffects,0);
            auto* Late=Panel(Clients.Last(),0,1);D->Test->TestTrue(TEXT("Late client receives broken state"),Late->IsBroken());D->Test->TestFalse(TEXT("Late client receives open collision"),Cast<UPrimitiveComponent>(Late->GetRootComponent())->IsCollisionEnabled());
            Remote[0]->ClientSetLocation(Left,FRotator::ZeroRotator);A->SetActorLocation(Left);A->GetCharacterMovement()->StopMovementImmediately();PreviousPlayer=Left;Next();return false;
        }
        if(Phase==7)
        {
            if(Now-At<.7)return false;CA->AddMovementInput(FVector(1,0,0),1);
            if(CrossesOpening(PreviousPlayer,A->GetActorLocation(),P->GetActorLocation()))PlayerCrossed=true;PreviousPlayer=A->GetActorLocation();
            if(A->GetActorLocation().X<P->GetActorLocation().X+120)return false;
            D->Test->TestTrue(TEXT("Player actually walks through panel opening"),PlayerCrossed);A->GetCharacterMovement()->StopMovementImmediately();
            A->SetActorLocation(P->GetActorLocation()+FVector(0,-450,-26));Remote[0]->ClientSetLocation(A->GetActorLocation(),FRotator::ZeroRotator);
            B->SetActorLocation(P->GetActorLocation()+FVector(0,450,-26));Remote[1]->ClientSetLocation(B->GetActorLocation(),FRotator::ZeroRotator);
            Next();return false;
        }
        if(Phase==8)
        {
            auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(Server);if(Nav->IsNavigationBuildInProgress() || Now-At<.5)return false;
            auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(Server,NavLeft,NavRight);
            if(!Path || !Path->IsValid() || Path->IsPartial()){D->Test->AddError(TEXT("No complete path through opened section"));return true;}
            D->Test->TestTrue(TEXT("Opening shortens path through barrier"),Path->GetPathLength()<DetourLength-50);
            auto* Bot=Server->SpawnActor<AMarau6Bot>(Left,FRotator::ZeroRotator);if(!Bot){D->Test->AddError(TEXT("Could not spawn navigation test bot"));return true;}Bot->SetActorTickEnabled(false);TestBot=Bot;PreviousBot=Left;
            auto* AI=Cast<AAIController>(Bot->GetController());if(!AI){D->Test->AddError(TEXT("Navigation test bot lacks controller"));return true;}AI->MoveToLocation(NavRight,15,false,true,false,false);Next();return false;
        }
        if(Phase==9)
        {
            auto* Bot=TestBot.Get();if(!Bot)return true;
            if(CrossesOpening(PreviousBot,Bot->GetActorLocation(),P->GetActorLocation()))BotCrossed=true;PreviousBot=Bot->GetActorLocation();
            if(FVector::Dist2D(Bot->GetActorLocation(),Right)>45)return false;
            D->Test->TestTrue(TEXT("Bot actually traverses opened panel"),BotCrossed);if(auto* AI=Bot->GetController())AI->Destroy();Bot->Destroy();
            Panel(Server,0,0)->TakeDamage(70,FDamageEvent(),nullptr,nullptr);D->Test->TestEqual(TEXT("Generic server damage accepted"),Panel(Server,0,0)->GetHealth(),130.f);
            for(int32 N=0;N<3;N++)if(auto* S=Panel(Server,0,N))S->TakeDamage(1000,FDamageEvent(),nullptr,nullptr);
            D->Test->TestTrue(TEXT("All sections can remove whole barrier"),Panel(Server,0,0)->IsBroken() && Panel(Server,0,1)->IsBroken() && Panel(Server,0,2)->IsBroken());
            B->TakeDamage(100,FDamageEvent(),Remote[0],A);A->SetActorLocation(FVector(1400,0,96));A->GetCharacterMovement()->StopMovementImmediately();Remote[0]->ClientSetLocation(A->GetActorLocation(),FRotator::ZeroRotator);Next();return false;
        }
        if(Phase==10){if(Remote[0]->IsInRaid() || Remote[1]->IsInRaid() || !Bridge->IsSettled())return false;Local[0]->ServerNextRound();Next();return false;}
        if(Phase==11)
        {
            if(Round==Bridge->GetRoundId() || !Bridge->CanMutate() || Panel(Clients[0],0,1)->IsBroken())return false;
            auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(Server);
            D->Test->TestFalse(TEXT("Round readiness waits for nav dirty queue"),Nav->HasDirtyAreasQueued() || Nav->IsNavigationBuildInProgress());
            auto* RestoredPath=UNavigationSystemV1::FindPathToLocationSynchronously(Server,NavLeft,NavRight);
            D->Test->TestTrue(TEXT("Ready new round restores navigation detour"),RestoredPath && RestoredPath->IsValid() && RestoredPath->GetPathLength()>FVector::Dist2D(Left,Right)+60);
            for(TActorIterator<AMarau6DestructibleSection> It(Server);It;++It)D->Test->TestEqual(TEXT("New round restores every panel"),It->GetHealth(),200.f);
            D->Test->TestTrue(TEXT("New round restores client collision"),Cast<UPrimitiveComponent>(Panel(Clients[0],0,1)->GetRootComponent())->IsCollisionEnabled());
            return true;
        }
        return false;
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6DestructionPIE,"EditorLifecycle.DestructionOnlineAndTeardown",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6DestructionPIE::RunTest(const FString&)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("Marau6Integration")) || !FParse::Param(FCommandLine::Get(),TEXT("Marau6DestructionTest"))){AddError(TEXT("Requires isolated launcher -PIE -Destruction"));return false;}
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/FirstPerson/Lvl_FirstPerson"));auto D=MakeShared<FSectionPIEData>(this);
    D->Settings->SetPlayNetMode(PIE_Client);D->Settings->SetPlayNumberOfClients(2);D->Settings->SetRunUnderOneProcess(true);D->Settings->NewWindowWidth=1050;D->Settings->NewWindowHeight=850;
    FRequestPlaySessionParams P;P.EditorPlaySettings=D->Settings.Get();P.bAllowOnlineSubsystem=false;
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(P));ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(4));ADD_LATENT_AUTOMATION_COMMAND(FSectionPIEFlow(D));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2));return true;
}
#endif
