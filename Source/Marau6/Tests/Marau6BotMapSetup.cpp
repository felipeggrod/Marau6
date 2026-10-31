#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/RecastNavMesh.h"
#include "Builders/CubeBuilder.h"
#include "ActorFactories/ActorFactory.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Misc/Paths.h"

namespace
{
class FBotsNavigationSave : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;double Started=FPlatformTime::Seconds();bool Requested=false;
public:
    explicit FBotsNavigationSave(FAutomationTestBase* T):Test(T){}
    virtual bool Update()override
    {
        UWorld* W=GEditor->GetEditorWorldContext().World();auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(W);
        if(!Nav)return true;
        if(!Requested)
        {
            if(Nav->IsNavigationBuildingLocked(static_cast<uint8>(~ENavigationBuildLock::NoUpdateInEditor)))
            {if(FPlatformTime::Seconds()-Started<90)return false;Test->AddError(TEXT("Navigation remained locked after loading"));return true;}
            Nav->Build();Requested=true;return false;
        }
        if(Nav->IsNavigationBuildInProgress()){if(FPlatformTime::Seconds()-Started<90)return false;Test->AddError(TEXT("Navigation build timeout"));return true;}
        int32 Projected=0;
        for(TActorIterator<ATargetPoint> It(W);It;++It)if(It->ActorHasTag(TEXT("Marau6BotSpawn")))
        {
            FNavLocation P;if(Nav->ProjectPointToNavigation(It->GetActorLocation(),P,FVector(100,100,800))){It->SetActorLocation(P.Location);++Projected;Test->AddInfo(FString::Printf(TEXT("Bot marker %s"),*P.Location.ToString()));}
        }
        Test->TestEqual(TEXT("Three navigable markers before saving"),Projected,3);
        if(Projected==3)Test->TestTrue(TEXT("Save prepared arena"),FEditorFileUtils::SaveMap(W,FPaths::ProjectContentDir()/TEXT("FirstPerson/Lvl_FirstPerson.umap")));
        return true;
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6BotMapSetup,"EditorSetup.PrepareBotArena",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6BotMapSetup::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/FirstPerson/Lvl_FirstPerson"));UWorld* W=GEditor->GetEditorWorldContext().World();
    ANavMeshBoundsVolume* Bounds=nullptr;for(TActorIterator<ANavMeshBoundsVolume> It(W);It;++It){Bounds=*It;break;}
    if(!Bounds)Bounds=W->SpawnActor<ANavMeshBoundsVolume>();
    if(!TestNotNull(TEXT("Navigation bounds"),Bounds))return false;
    Bounds->SetActorLabel(TEXT("Marau6 Bot Navigation"));
    auto* Cube=NewObject<UCubeBuilder>();Cube->X=7000;Cube->Y=7000;Cube->Z=2000;UActorFactory::CreateBrushForVolumeActor(Bounds,Cube);Bounds->SetActorLocation(FVector(0,0,300));
    TArray<ATargetPoint*> Old;for(TActorIterator<ATargetPoint> It(W);It;++It)if(It->ActorHasTag(TEXT("Marau6BotSpawn")))Old.Add(*It);for(auto* A:Old)W->DestroyActor(A);
    const FVector Points[]={FVector(-1200,-900,300),FVector(-1200,900,300),FVector(500,1200,300)};
    for(int32 I=0;I<3;++I){auto* A=W->SpawnActor<ATargetPoint>(Points[I],(-Points[I]).Rotation());A->Tags.Add(TEXT("Marau6BotSpawn"));A->SetActorLabel(FString::Printf(TEXT("Marau6 Bot Spawn %d"),I+1));}
    auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(W);
    if(!TestNotNull(TEXT("Navigation system in arena"),Nav))return false;
    Nav->OnNavigationBoundsUpdated(Bounds);
    if(auto* Mesh=Cast<ARecastNavMesh>(Nav->GetDefaultNavDataInstance(FNavigationSystem::Create))){Mesh->AgentRadius=34;Mesh->AgentHeight=192;}
    ADD_LATENT_AUTOMATION_COMMAND(FBotsNavigationSave(this));return true;
}
#endif
