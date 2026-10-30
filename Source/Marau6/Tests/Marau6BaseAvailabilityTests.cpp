#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Marau6Persistence.h"
#include "Marau6PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6BaseAvailabilityTest,"Marau6.Persistence.UnavailableFeedback",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6BaseAvailabilityTest::RunTest(const FString& Parameters)
{
    auto* W=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);W->InitializeActorsForPlay(FURL());
    auto* Class=LoadClass<AMarau6PlayerController>(nullptr,TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonPlayerController.BP_FirstPersonPlayerController_C"));
    auto* PC=W->SpawnActor<AMarau6PlayerController>(Class);
    if(!TestNotNull(TEXT("Player controller"),PC))return false;
    auto* Bridge=W->GetSubsystem<UMarau6Persistence>();
    TestFalse(TEXT("Backend is not ready in this fixture"),Bridge->CanMutate());
    Bridge->SelectProfile(PC,1);
    TestTrue(TEXT("Profile selection explains backend unavailability"),PC->GetBaseMessage().Contains(TEXT("backend")));
    Bridge->EnterRaid(PC);
    TestTrue(TEXT("B explains why entry is blocked"),PC->GetBaseMessage().Contains(TEXT("backend")));
    Bridge->NextRound(PC);
    TestTrue(TEXT("N explains why round is blocked"),PC->GetBaseMessage().Contains(TEXT("backend")));
    TestFalse(TEXT("No participation without backend"),PC->IsInRaid());
    GEngine->DestroyWorldContext(W);W->DestroyWorld(false);if(W->IsRooted())W->RemoveFromRoot();return true;
}
#endif
