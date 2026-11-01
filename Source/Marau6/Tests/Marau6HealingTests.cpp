#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Marau6Character.h"
#include "Marau6HealingComponent.h"
#include "Marau6InventoryComponent.h"
#include "Marau6ItemLedger.h"
#include "Marau6LootActor.h"
#include "Marau6ExtractionZone.h"
#include "Marau6Persistence.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "Engine/LocalPlayer.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/Package.h"

namespace
{
struct FHealingWorld
{
    UWorld* W;UMarau6ItemLedger* L;AMarau6Character* P;
    FHealingWorld(bool Persistent=false)
    {
        W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,Persistent?CreatePackage(*(TEXT("/Game/FirstPerson/Lvl_FirstPerson_HealTest_")+FGuid::NewGuid().ToString())):nullptr);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);W->InitializeActorsForPlay(FURL());L=W->GetSubsystem<UMarau6ItemLedger>();
        auto* Floor=W->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Floor);Floor->SetRootComponent(Box);Box->SetBoxExtent(FVector(3000,3000,10));Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();Floor->SetActorLocation(FVector(0,0,-10));
        auto* C=LoadClass<AMarau6Character>(nullptr,TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C"));
        P=W->SpawnActor<AMarau6Character>(C,FVector(0,0,96),FRotator::ZeroRotator);auto* PC=W->SpawnActor<APlayerController>();PC->Player=NewObject<ULocalPlayer>(GEngine);PC->Possess(P);P->DispatchBeginPlay();P->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
    ~FHealingWorld(){GEngine->DestroyWorldContext(W);W->DestroyWorld(false);if(W->IsRooted())W->RemoveFromRoot();}
    FGuid Kit(int32 Slot=0)
    {
        const FVector Position=P->GetActorLocation()+FVector(120,0,-66);auto* A=W->SpawnActor<AMarau6LootActor>(Position,FRotator::ZeroRotator);L->RegisterSeed(A,EMarau6ItemKind::Supply);A->DispatchBeginPlay();const FGuid Id=A->GetItem().Id;
        P->GetController()->SetControlRotation((Position-P->GetPawnViewLocation()).Rotation());
        if(!L->Pickup(P->GetInventory(),A,Id))return FGuid();
        const auto* Item=P->GetInventory()->GetSnapshot().Items.FindByPredicate([Id](const FMarau6Item& I){return I.Id==Id;});
        if(Item && Item->Slot!=Slot)L->Move(P->GetInventory(),Id,Slot);return Id;
    }
    void Advance(float D){while(D>KINDA_SMALL_NUMBER){const float Step=FMath::Min(.05f,D);++GFrameCounter;W->Tick(LEVELTICK_TimeOnly,Step);W->GetTimerManager().Tick(Step);P->GetHealing()->UpdateHealing();D-=Step;}}
    FString Custody(FGuid Id){for(const auto& V:L->Export())if(V->AsObject()->GetStringField(TEXT("id"))==Id.ToString(EGuidFormats::DigitsWithHyphensLower))return V->AsObject()->GetStringField(TEXT("custody"));return TEXT("missing");}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6HealingFlowTest,"Marau6.Healing.DelayedConsumption",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6HealingFlowTest::RunTest(const FString&)
{
    FHealingWorld F;const FGuid Id=F.Kit(),Other=F.Kit(1);auto* H=F.P->GetHealing();
    if(!TestTrue(TEXT("Valid kit fixture"),Id.IsValid() && Other.IsValid()))return false;
    F.P->TakeDamage(60,FDamageEvent(),nullptr,nullptr);F.P->OpenInventoryResult();H->UseSelected();
    TestTrue(TEXT("Treatment begins"),H->IsHealing());const float Remaining=H->Remaining();H->ServerUse(Other);
    TestTrue(TEXT("Repeated request cannot restart treatment"),FMath::IsNearlyEqual(Remaining,H->Remaining(),.05f));
    F.Advance(3.8f);TestEqual(TEXT("No early healing"),F.P->GetHealth(),40.f);TestTrue(TEXT("Kit held until completion"),F.P->GetInventory()->ItemAt(0)!=nullptr);
    F.Advance(.3f);TestEqual(TEXT("Completed treatment restores forty HP"),F.P->GetHealth(),80.f);
    TestFalse(TEXT("Treatment ends"),H->IsHealing());TestNull(TEXT("Exactly selected kit removed"),F.P->GetInventory()->ItemAt(0));TestTrue(TEXT("Other kit retained"),F.P->GetInventory()->ItemAt(1) && F.P->GetInventory()->ItemAt(1)->Id==Other);
    TestEqual(TEXT("Terminal record retained"),F.Custody(Id),FString(TEXT("consumed")));const int32 Count=F.L->GetItemCount();
    H->ServerUse(Id);H->UpdateHealing();F.Advance(4.1f);TestEqual(TEXT("Consumed identity cannot heal again"),F.P->GetHealth(),80.f);TestEqual(TEXT("Consumption never creates identities"),F.L->GetItemCount(),Count);
    TestFalse(TEXT("Consumed identity cannot be moved"),F.L->Move(F.P->GetInventory(),Id,2));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6HealingRulesTest,"Marau6.Healing.InputAndRestrictions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6HealingRulesTest::RunTest(const FString&)
{
    FHealingWorld F;const FGuid Id=F.Kit(8);auto* H=F.P->GetHealing();H->ServerUse(Id);TestFalse(TEXT("Full health rejects kit without consuming"),H->IsHealing());
    F.P->TakeDamage(10,FDamageEvent(),nullptr,nullptr);const FGuid Rifle=F.P->GetInventory()->ItemAt(10)->Id;H->ServerUse(Rifle);TestFalse(TEXT("Rifle is not medicine"),H->IsHealing());H->ServerUse(FGuid::NewGuid());TestFalse(TEXT("Unknown or foreign identity rejected"),H->IsHealing());
    F.P->ProcessEvent(F.P->FindFunction(TEXT("ServerFire")),nullptr);F.P->ProcessEvent(F.P->FindFunction(TEXT("ServerReload")),nullptr);H->ServerUse(Id);TestFalse(TEXT("Reload prevents treatment start"),H->IsHealing());F.Advance(2.1f);
    F.P->ProcessEvent(F.P->FindFunction(TEXT("ServerFire")),nullptr);F.P->OpenInventoryResult();H->ServerUse(Id);TestTrue(TEXT("Safe pocket supply can heal"),H->IsHealing());TestFalse(TEXT("Accepted treatment closes inventory"),F.P->IsInventoryOpen());
    F.Advance(.3f);const int32 Ammo=F.P->GetMagazineAmmo();F.P->ProcessEvent(F.P->FindFunction(TEXT("ServerFire")),nullptr);F.P->ProcessEvent(F.P->FindFunction(TEXT("ServerReload")),nullptr);
    TestEqual(TEXT("Server rejects firing during treatment"),F.P->GetMagazineAmmo(),Ammo);TestFalse(TEXT("Server rejects reload during treatment"),F.P->IsReloading());F.Advance(4.1f);
    TestEqual(TEXT("Healing clamps to max100"),F.P->GetHealth(),100.f);TestEqual(TEXT("Safe supply remains consumed"),F.Custody(Id),FString(TEXT("consumed")));
    F.P->ProcessEvent(F.P->FindFunction(TEXT("ServerReload")),nullptr);TestTrue(TEXT("Reload works after treatment"),F.P->IsReloading());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6HealingCancelTest,"Marau6.Healing.Interruptions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6HealingCancelTest::RunTest(const FString&)
{
    for(int32 Case=0;Case<9;++Case)
    {
        FHealingWorld F;const FGuid Id=F.Kit();auto* H=F.P->GetHealing();F.P->TakeDamage(60,FDamageEvent(),nullptr,nullptr);H->ServerUse(Id);TestTrue(FString::Printf(TEXT("Interruption %d starts active"),Case),H->IsHealing());
        if(Case==0)H->ServerCancel();
        if(Case==1)F.P->TakeDamage(5,FDamageEvent(),nullptr,nullptr);
        if(Case==2){struct{float Right=1,Forward=0;} Move;F.P->ProcessEvent(F.P->FindFunction(TEXT("DoMove")),&Move);}
        if(Case==3)F.P->ProcessEvent(F.P->FindFunction(TEXT("DoJumpStart")),nullptr);
        if(Case==4)F.L->Move(F.P->GetInventory(),Id,3);
        if(Case==5)F.L->Drop(F.P->GetInventory(),Id);
        if(Case==6){F.W->SpawnActor<AMarau6ExtractionZone>(FVector::ZeroVector,FRotator::ZeroRotator);F.P->Tick(.01f);}
        if(Case==7)F.P->TakeDamage(100,FDamageEvent(),nullptr,nullptr);
        if(Case==8)F.P->GetController()->UnPossess();
        H->UpdateHealing();TestFalse(FString::Printf(TEXT("Interruption %d cancels treatment"),Case),H->IsHealing());
        const float HP=F.P->GetHealth();F.Advance(4.2f);TestEqual(FString::Printf(TEXT("Interruption %d never grants delayed HP"),Case),F.P->GetHealth(),HP);
        TestNotEqual(FString::Printf(TEXT("Interruption %d does not consume kit"),Case),F.Custody(Id),FString(TEXT("consumed")));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6HealingDurabilityTest,"Marau6.Healing.DurableConsumption",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6HealingDurabilityTest::RunTest(const FString&)
{
    FHealingWorld F(true);auto* Bridge=F.W->GetSubsystem<UMarau6Persistence>();Bridge->bEnabled=true;Bridge->bReady=true;Bridge->bSending=true;
    FMarau6Item Kit;Kit.Id=FGuid::NewGuid();Kit.Kind=EMarau6ItemKind::Supply;Kit.Slot=8;
    TestTrue(TEXT("Import safe kit fixture"),F.L->Import(F.P->GetInventory(),FGuid::NewGuid(),{Kit}));
    const FString Root=FPaths::ProjectSavedDir()/TEXT("Automation/Healing");IFileManager::Get().MakeDirectory(*Root,true);const FString Blocker=Root/TEXT("blocked-parent");FFileHelper::SaveStringToFile(TEXT("file"),*Blocker);
    Bridge->JournalPath=Blocker/TEXT("journal.json");Bridge->RaidId=FGuid::NewGuid().ToString();
    F.P->TakeDamage(60,FDamageEvent(),nullptr,nullptr);F.P->GetHealing()->ServerUse(Kit.Id);TestTrue(TEXT("Treatment before storage failure"),F.P->GetHealing()->IsHealing());
    AddExpectedError(TEXT("Falha ao gravar diario"),EAutomationExpectedErrorFlags::Contains,1);F.Advance(4.1f);
    TestEqual(TEXT("Failed consumption grants no HP"),F.P->GetHealth(),40.f);TestTrue(TEXT("Failed consumption restores exact safe item"),F.P->GetInventory()->ItemAt(8) && F.P->GetInventory()->ItemAt(8)->Id==Kit.Id);TestEqual(TEXT("Failed consumption rolls back custody"),F.Custody(Kit.Id),FString(TEXT("inventory")));
    Bridge->bFatal=false;Bridge->bReady=true;Bridge->JournalPath=Root/TEXT("journal.json");F.P->GetHealing()->ServerUse(Kit.Id);F.Advance(4.1f);
    TestEqual(TEXT("Retry applies one forty HP effect"),F.P->GetHealth(),80.f);TestEqual(TEXT("Successful retry keeps terminal GUID"),F.Custody(Kit.Id),FString(TEXT("consumed")));TestNull(TEXT("Used safe kit cannot be restored by inventory sync"),F.P->GetInventory()->ItemAt(8));
    F.P->GetHealing()->UpdateHealing();TestFalse(TEXT("Repeated consume denied"),F.L->ConsumeForHealing(F.P->GetInventory(),Kit.Id));TestEqual(TEXT("Repeated completion cannot heal twice"),F.P->GetHealth(),80.f);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6WearJournalTest,"Marau6.Combat.WearJournalRollback",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6WearJournalTest::RunTest(const FString&)
{
    FHealingWorld F(true);auto* Bridge=F.W->GetSubsystem<UMarau6Persistence>();Bridge->bEnabled=true;Bridge->bReady=true;Bridge->bSending=true;
    FMarau6Item Rifle;Rifle.Id=FGuid::NewGuid();Rifle.Kind=EMarau6ItemKind::Rifle;Rifle.Slot=10;Rifle.Magazine=10;Rifle.Reserve=40;Rifle.Durability=250;
    TestTrue(TEXT("Import rifle with one remaining shot of condition"),F.L->Import(F.P->GetInventory(),FGuid::NewGuid(),{Rifle}));
    const FString Root=FPaths::ProjectSavedDir()/TEXT("Automation/Wear");IFileManager::Get().MakeDirectory(*Root,true);const FString Blocker=Root/TEXT("blocked-parent");FFileHelper::SaveStringToFile(TEXT("file"),*Blocker);
    Bridge->JournalPath=Blocker/TEXT("journal.json");Bridge->RaidId=FGuid::NewGuid().ToString();
    auto* Target=F.W->SpawnActor<AMarau6Character>(F.P->GetClass(),FVector(500,0,96),FRotator::ZeroRotator);F.P->GetController()->SetControlRotation(FRotator::ZeroRotator);
    AddExpectedError(TEXT("Falha ao gravar diario"),EAutomationExpectedErrorFlags::Contains,1);
    F.P->ProcessEvent(F.P->FindFunction(TEXT("ServerFire")),nullptr);
    TestEqual(TEXT("Failed journal keeps exact condition"),F.P->GetRifleDurability(),250);TestEqual(TEXT("Failed journal keeps item ammo"),F.P->GetInventory()->ItemAt(10)->Magazine,10);
    TestEqual(TEXT("Failed journal keeps replicated ammo"),F.P->GetMagazineAmmo(),10);TestEqual(TEXT("Unpaid shot causes no damage"),Target->GetHealth(),100.f);
    Bridge->bFatal=false;Bridge->bReady=true;Bridge->JournalPath=Root/TEXT("journal.json");
    F.P->ProcessEvent(F.P->FindFunction(TEXT("ServerFire")),nullptr);
    TestEqual(TEXT("Retry wears exactly once"),F.P->GetRifleDurability(),0);TestEqual(TEXT("Retry spends exactly one round"),F.P->GetMagazineAmmo(),9);TestEqual(TEXT("Last working shot applies damage"),Target->GetHealth(),75.f);
    F.Advance(.3f);F.P->ProcessEvent(F.P->FindFunction(TEXT("ServerFire")),nullptr);TestEqual(TEXT("Zero blocks further damage"),Target->GetHealth(),75.f);
    FString Journal;FFileHelper::LoadFileToString(Journal,*Bridge->JournalPath);TestTrue(TEXT("Durability zero present in durable command"),Journal.Contains(TEXT("durability")));
    return true;
}
#endif
