#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Marau6Bot.h"
#include "Marau6Persistence.h"
#include "Marau6InventoryComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/Package.h"
#include "Marau6Character.h"
#include "Marau6ItemLedger.h"
#include "Marau6LootActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/CharacterMovementComponent.h"
namespace
{
struct FBotWorld
{
    UWorld* W;
    FBotWorld(bool Persistent=false){W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,Persistent?CreatePackage(*(TEXT("/Game/FirstPerson/Lvl_FirstPerson_BotTest_")+FGuid::NewGuid().ToString())):nullptr);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);W->InitializeActorsForPlay(FURL());}
    ~FBotWorld(){GEngine->DestroyWorldContext(W);W->DestroyWorld(false);if(W->IsRooted())W->RemoveFromRoot();}
    AMarau6Character* Player(FVector V)
    {
        auto* C=LoadClass<AMarau6Character>(nullptr,TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C"));
        FActorSpawnParameters S;S.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* P=W->SpawnActor<AMarau6Character>(C,V,FRotator::ZeroRotator,S);W->SpawnActor<APlayerController>()->Possess(P);P->GetCharacterMovement()->SetMovementMode(MOVE_Flying);P->DispatchBeginPlay();return P;
    }
    AMarau6Bot* Bot(){auto* B=W->SpawnActor<AMarau6Bot>(FVector(0,0,100),FRotator::ZeroRotator);B->DispatchBeginPlay();B->SetActorTickEnabled(false);return B;}
    void Advance(float D){while(D>KINDA_SMALL_NUMBER){const float Step=FMath::Min(D,.1f);++GFrameCounter;W->Tick(LEVELTICK_TimeOnly,Step);D-=Step;}}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6BotBehaviorTest,"Marau6.Bots.ServerBehavior",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6BotBehaviorTest::RunTest(const FString&)
{
    FBotWorld F;auto* B=F.Bot();auto* P=F.Player(FVector(500,0,100));
    TestTrue(TEXT("Visible active player detected"),B->CanSee(P));
    P->SetActorLocation(FVector(-500,0,100));TestFalse(TEXT("Behind cone is unseen"),B->CanSee(P));
    P->SetActorLocation(FVector(2100,0,100));TestFalse(TEXT("Beyond sight range unseen"),B->CanSee(P));
    P->SetActorLocation(FVector(500,0,100));P->SetAtBase(true);TestFalse(TEXT("Base players excluded"),B->CanSee(P));F.W->SpawnActor<APlayerStart>(FVector(500,0,100),FRotator::ZeroRotator);P->SetAtBase(false);P->SetActorLocation(FVector(500,0,100));
    auto* Wall=F.W->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Box);Box->SetBoxExtent(FVector(20,200,300));Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();Wall->SetActorLocation(FVector(250,0,100));
    TestFalse(TEXT("Wall blocks perception"),B->CanSee(P));Wall->Destroy();
    B->StepAI();TestEqual(TEXT("Reaction prevents immediate shot"),B->GetShotCount(),0);
    F.Advance(1.1f);B->StepAI();TestEqual(TEXT("First accepted shot after reaction"),B->GetShotCount(),1);
    B->StepAI();TestEqual(TEXT("Repeated same-tick shot rejected"),B->GetShotCount(),1);
    const FVector Seen=B->GetLastSeen();P->SetActorLocation(FVector(-600,0,100));B->StepAI();
    TestTrue(TEXT("Hidden target does not reveal new location"),B->GetLastSeen().Equals(Seen));
    TestEqual(TEXT("Last-seen investigation state"),B->GetBotState(),EMarau6BotState::Investigate);
    F.Advance(5.1f);B->StepAI();TestEqual(TEXT("Memory expires back to patrol"),B->GetBotState(),EMarau6BotState::Patrol);
    TestFalse(TEXT("No navigation data never falls back to direct movement"),B->Navigate(FVector(1000,0,100)));
    FDamageEvent E;const int32 Before=F.W->GetSubsystem<UMarau6ItemLedger>()->GetItemCount();
    TestEqual(TEXT("Player rifle damages bot"),B->TakeDamage(25,E,P->GetController(),P),25.f);
    TestEqual(TEXT("One bot health model"),B->GetHealth(),75.f);
    B->TakeDamage(100,E,P->GetController(),P);const FGuid Reward=B->GetLootId();
    TestFalse(TEXT("Bot dies"),B->IsAlive());B->TakeDamage(100,E,P->GetController(),P);B->StepAI();
    TestEqual(TEXT("Death creates one reward only"),F.W->GetSubsystem<UMarau6ItemLedger>()->GetItemCount(),Before+1);
    TestEqual(TEXT("Death retry preserves reward identity"),B->GetLootId(),Reward);
    TestEqual(TEXT("Dead bot cannot shoot"),B->GetShotCount(),1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6BotGrantTest,"Marau6.Bots.StableLoot",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6BotGrantTest::RunTest(const FString&)
{
    FBotWorld F;auto* L=F.W->GetSubsystem<UMarau6ItemLedger>();const FGuid Id=FGuid::NewGuid();
    TestTrue(TEXT("Grant one stable reward"),L->GrantWorld(Id,EMarau6ItemKind::Scrap,FVector(0,0,50)));
    TestTrue(TEXT("Replay same grant is idempotent"),L->GrantWorld(Id,EMarau6ItemKind::Scrap,FVector(0,0,50)));
    TestEqual(TEXT("One record after replay"),L->GetItemCount(),1);
    int32 Count=0;for(TActorIterator<AMarau6LootActor> It(F.W);It;++It)if(It->GetItem().Id==Id)++Count;
    TestEqual(TEXT("One collectable world instance"),Count,1);
    TestFalse(TEXT("Reusing reward ID for other item kind rejected"),L->GrantWorld(Id,EMarau6ItemKind::Rifle,FVector::ZeroVector));
    TestFalse(TEXT("Invalid reward identity rejected"),L->GrantWorld(FGuid(),EMarau6ItemKind::Scrap,FVector::ZeroVector));
    auto* P=F.Player(FVector(-100,0,146));AMarau6LootActor* Loot=nullptr;
    for(TActorIterator<AMarau6LootActor> It(F.W);It;++It)if(It->GetItem().Id==Id)Loot=*It;
    P->GetController()->SetControlRotation((FVector(0,0,50)-P->GetPawnViewLocation()).Rotation());
    TestTrue(TEXT("Reward collected through normal ledger"),L->Pickup(P->GetInventory(),Loot,Id));
    TestTrue(TEXT("Grant replay after pickup does not respawn reward"),L->GrantWorld(Id,EMarau6ItemKind::Scrap,FVector(0,0,50)));
    TestTrue(TEXT("Collected identity remains owned"),P->GetInventory()->ItemAt(0) && P->GetInventory()->ItemAt(0)->Id==Id);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6BotDurabilityTest,"Marau6.Bots.DurableReward",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6BotDurabilityTest::RunTest(const FString&)
{
    FBotWorld F(true);auto* B=F.Bot();auto* Bridge=F.W->GetSubsystem<UMarau6Persistence>();auto* L=F.W->GetSubsystem<UMarau6ItemLedger>();
    Bridge->bEnabled=true;Bridge->bReady=true;
    const FString Root=FPaths::ProjectSavedDir()/TEXT("Automation/BotReward");IFileManager::Get().MakeDirectory(*Root,true);
    const FString Blocker=Root/TEXT("blocked-parent");FFileHelper::SaveStringToFile(TEXT("file, not folder"),*Blocker);
    Bridge->JournalPath=Blocker/TEXT("journal.json");Bridge->RaidId=FGuid::NewGuid().ToString();
    AddExpectedError(TEXT("Falha ao gravar diario"),EAutomationExpectedErrorFlags::Contains,1);
    TestEqual(TEXT("Failed durable grant does not confirm death"),B->TakeDamage(100,FDamageEvent(),nullptr,nullptr),0.f);
    TestEqual(TEXT("Bot remains alive on failed journal"),B->GetHealth(),100.f);
    TestEqual(TEXT("No unjournaled reward remains"),L->GetItemCount(),0);
    Bridge->bFatal=false;Bridge->bReady=true;Bridge->JournalPath=Root/TEXT("journal.json");
    const FGuid Id=B->GetLootId();TestEqual(TEXT("Retry after writer restored completes death"),B->TakeDamage(100,FDamageEvent(),nullptr,nullptr),100.f);
    TestEqual(TEXT("Retry uses original reward identity"),B->GetLootId(),Id);TestEqual(TEXT("Exactly one durable reward"),L->GetItemCount(),1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6BotReentrantTest,"Marau6.Bots.ReentrantDeath",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6BotReentrantTest::RunTest(const FString&)
{
    FBotWorld F;auto* B=F.Bot();float Nested=-1;
    const auto Handle=F.W->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([&](AActor* Actor){if(Actor->IsA<AMarau6LootActor>())Nested=B->TakeDamage(100,FDamageEvent(),nullptr,nullptr);}));
    TestEqual(TEXT("Outer lethal hit accepted once"),B->TakeDamage(100,FDamageEvent(),nullptr,nullptr),100.f);
    TestEqual(TEXT("Nested lethal hit during reward spawn rejected"),Nested,0.f);
    TestEqual(TEXT("One reward despite nested damage callback"),F.W->GetSubsystem<UMarau6ItemLedger>()->GetItemCount(),1);
    F.W->RemoveOnActorSpawnedHandler(Handle);return true;
}
#endif
