#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Editor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6BotMapTest,"Marau6.Bots.MapNavigation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6BotMapTest::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/FirstPerson/Lvl_FirstPerson"));UWorld* W=GEditor->GetEditorWorldContext().World();
    auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(W);
    if(!TestNotNull(TEXT("Arena navigation system"),Nav))return false;
    int32 Markers=0;for(TActorIterator<ATargetPoint> It(W);It;++It)if(It->ActorHasTag(TEXT("Marau6BotSpawn"))){++Markers;FNavLocation P;TestTrue(TEXT("Spawn marker is on navigation"),Nav->ProjectPointToNavigation(It->GetActorLocation(),P,FVector(100,100,650)));}
    TestEqual(TEXT("Exactly three authored bot markers"),Markers,3);
    TArray<FVector> Points;
    for(int32 X=-1500;X<=1500;X+=500)for(int32 Y=-1500;Y<=1500;Y+=500){FNavLocation P;if(Nav->ProjectPointToNavigation(FVector(X,Y,300),P,FVector(80,80,600)))Points.Add(P.Location);}
    bool Detour=false;
    for(int32 A=0;A<Points.Num() && !Detour;++A)for(int32 B=A+1;B<Points.Num() && !Detour;++B)
    {
        const FVector Start=Points[A]+FVector(0,0,96),End=Points[B]+FVector(0,0,96);FHitResult H;
        if(!W->SweepSingleByChannel(H,Start,End,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(32,90)))continue;
        auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(W,Points[A],Points[B]);
        if(Path && Path->IsValid() && !Path->IsPartial() && Path->PathPoints.Num()>2 && Path->GetPathLength()>FVector::Distance(Points[A],Points[B])*1.1)
        {Detour=true;AddInfo(FString::Printf(TEXT("Real arena detour %s -> %s, %d points, %.0f cm"),*Points[A].ToString(),*Points[B].ToString(),Path->PathPoints.Num(),Path->GetPathLength()));}
    }
    TestTrue(TEXT("A valid arena path goes around a real blocking obstacle"),Detour);return true;
}
#endif
