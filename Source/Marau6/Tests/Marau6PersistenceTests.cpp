#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Marau6Persistence.h"
#include "Marau6ItemLedger.h"
#include "Marau6Character.h"
#include "Marau6InventoryComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6PersistenceTest,"Marau6.Persistence.ImportAndJournal",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6PersistenceTest::RunTest(const FString& Parameters)
{
    auto* W=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);W->InitializeActorsForPlay(FURL());
    TestFalse(TEXT("Fixture explicitly uses transient combat mode"),UMarau6Persistence::UsesPersistentBase(W));
    auto* L=W->GetSubsystem<UMarau6ItemLedger>();
    auto* Class=LoadClass<AMarau6Character>(nullptr,TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C"));
    auto* P=W->SpawnActor<AMarau6Character>(Class);
    FMarau6Item Item;Item.Id=FGuid::NewGuid();Item.Kind=EMarau6ItemKind::Rifle;Item.Slot=10;Item.Magazine=3;Item.Reserve=17;
    const FGuid Owner=FGuid::NewGuid();
    TestTrue(TEXT("Import reserved equipment"),L->Import(P->GetInventory(),Owner,{Item}));
    TestEqual(TEXT("No starting-rifle duplication"),L->GetItemCount(),1);
    TestEqual(TEXT("Imported identity"),P->GetInventory()->ItemAt(10)->Id,Item.Id);
    const auto Snapshot=L->Export();TestEqual(TEXT("One exported identity"),Snapshot.Num(),1);
    TestEqual(TEXT("Ammo exported unchanged"),Snapshot[0]->AsObject()->GetIntegerField(TEXT("magazine")),3);
    const FString File=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Automation/persistence-journal-test.json"));
    TestTrue(TEXT("Durable atomic write"),UMarau6Persistence::AtomicWrite(File,TEXT("first")));
    TestTrue(TEXT("Replace durable journal"),UMarau6Persistence::AtomicWrite(File,TEXT("second")));
    FString Read;FFileHelper::LoadFileToString(Read,*File);TestEqual(TEXT("Latest complete journal"),Read,TEXT("second"));
    TestFalse(TEXT("Reject file used as parent"),UMarau6Persistence::AtomicWrite(File+TEXT("/invalid.json"),TEXT("bad")));
    GEngine->DestroyWorldContext(W);W->DestroyWorld(false);if(W->IsRooted())W->RemoveFromRoot();return true;
}
#endif
