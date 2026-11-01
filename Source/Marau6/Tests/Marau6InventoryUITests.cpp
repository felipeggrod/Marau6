#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Marau6PlayerController.h"
#include "Marau6Character.h"
#include "Marau6InventoryUIModel.h"
#include "Marau6InventoryComponent.h"
#include "Marau6ItemLedger.h"
#include "Marau6HealingComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/DamageEvents.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
struct FUIWorld
{
    UWorld* W;AMarau6Character* C;AMarau6PlayerController* PC;UMarau6ItemLedger* L;
    FUIWorld()
    {
        W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);W->InitializeActorsForPlay(FURL());
        auto* Floor=W->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Floor);Floor->SetRootComponent(Box);Box->SetBoxExtent(FVector(3000,3000,10));Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();Floor->SetActorLocation(FVector(0,0,-10));
        auto* PawnClass=LoadClass<AMarau6Character>(nullptr,TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C"));
        auto* ControllerClass=LoadClass<AMarau6PlayerController>(nullptr,TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonPlayerController.BP_FirstPersonPlayerController_C"));
        C=W->SpawnActor<AMarau6Character>(PawnClass,FVector(0,0,96),FRotator::ZeroRotator);PC=W->SpawnActor<AMarau6PlayerController>(ControllerClass);PC->Player=NewObject<ULocalPlayer>(GEngine);PC->Possess(C);C->DispatchBeginPlay();C->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        L=W->GetSubsystem<UMarau6ItemLedger>();FMarau6Item Rifle;Rifle.Id=FGuid::NewGuid();Rifle.Kind=EMarau6ItemKind::Rifle;Rifle.Slot=10;Rifle.Magazine=3;Rifle.Reserve=17;Rifle.Durability=7500;
        FMarau6Item Kit;Kit.Id=FGuid::NewGuid();Kit.Kind=EMarau6ItemKind::Supply;Kit.Slot=0;FMarau6Item Safe=Kit;Safe.Id=FGuid::NewGuid();Safe.Slot=8;
        check(L->Import(C->GetInventory(),FGuid::NewGuid(),{Rifle,Kit,Safe}));C->OpenInventoryResult();PC->RefreshInventoryContext();
    }
    ~FUIWorld(){GEngine->DestroyWorldContext(W);W->DestroyWorld(false);if(W->IsRooted())W->RemoveFromRoot();}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6UIRulesTest,"Marau6.Inventory.UIRules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6UIRulesTest::RunTest(const FString&)
{
    FUIWorld F;auto Items=F.PC->GetInventoryUIItems();
    TestTrue(TEXT("Rifle to empty backpack"),FMarau6InventoryUIModel::MoveReason(Items,10,1,false).IsEmpty());
    TestTrue(TEXT("Supply can swap with safe supply"),FMarau6InventoryUIModel::MoveReason(Items,0,8,false).IsEmpty());
    TestTrue(TEXT("Safe rule explains rejection"),FMarau6InventoryUIModel::MoveReason(Items,10,9,false).Contains(TEXT("Rifles")));
    TestTrue(TEXT("Incompatible reverse swap rejected"),!FMarau6InventoryUIModel::MoveReason(Items,10,0,false).IsEmpty());
    TestFalse(TEXT("Raid cannot access stash"),FMarau6InventoryUIModel::ValidSlot(100,false));TestTrue(TEXT("Base stash32"),FMarau6InventoryUIModel::ValidSlot(131,true));TestFalse(TEXT("Stash out of bounds"),FMarau6InventoryUIModel::ValidSlot(132,true));
    TestTrue(TEXT("Rifle info includes exact worn condition"),FMarau6InventoryUIModel::Details(Items[0]).Len()>0);
    const auto* Rifle=F.PC->InventoryUIItemAt(10);TestTrue(TEXT("Rifle detail retains ammo and condition"),FMarau6InventoryUIModel::Details(*Rifle).Contains(TEXT("75.0%")) && FMarau6InventoryUIModel::Details(*Rifle).Contains(TEXT("17")));
    TestTrue(TEXT("Supply explains duration"),FMarau6InventoryUIModel::Details(*F.PC->InventoryUIItemAt(8)).Contains(TEXT("4 segundos")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6UIRequestTest,"Marau6.Inventory.UIRequests",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6UIRequestTest::RunTest(const FString&)
{
    FUIWorld F;FMarau6InventoryUIRequest R;const auto Original=*F.PC->InventoryUIItemAt(10);
    TestFalse(TEXT("Empty slot cannot drag"),F.PC->CaptureInventoryDrag(1,R));TestTrue(TEXT("Capture rifle without changing inventory"),F.PC->CaptureInventoryDrag(10,R));
    TestEqual(TEXT("Capture keeps UUID"),F.PC->InventoryUIItemAt(10)->Id,Original.Id);
    TestFalse(TEXT("Local safe rejection"),F.PC->SubmitInventoryDrop(R,9));TestTrue(TEXT("Rejection message retained"),F.PC->GetInventoryNotice().Contains(TEXT("Rifles")));
    TestTrue(TEXT("Move accepted through correlated RPC"),F.PC->SubmitInventoryDrop(R,1));TestNull(TEXT("Source empty only after commit"),F.PC->InventoryUIItemAt(10));
    TestEqual(TEXT("Moved UUID"),F.PC->InventoryUIItemAt(1)->Id,Original.Id);TestEqual(TEXT("Moved condition"),F.PC->InventoryUIItemAt(1)->Durability,7500);TestEqual(TEXT("Moved ammo"),F.PC->InventoryUIItemAt(1)->Magazine,3);
    const int32 Revision=F.PC->InventoryUIRevision();R.Destination=1;F.PC->ServerInventoryMoveUI(R);TestEqual(TEXT("Repeated request cannot move again"),F.PC->InventoryUIRevision(),Revision);
    R.Destination=2;F.PC->ServerInventoryMoveUI(R);TestEqual(TEXT("Changed replay refused"),F.PC->InventoryUIRevision(),Revision);
    FMarau6InventoryUIRequest Wrong;TestTrue(TEXT("New capture"),F.PC->CaptureInventoryDrag(1,Wrong));Wrong.Destination=2;Wrong.DestinationItem=FGuid::NewGuid();F.PC->ServerInventoryMoveUI(Wrong);
    TestEqual(TEXT("Wrong destination identity refused"),F.PC->InventoryUIRevision(),Revision);
    Wrong.RequestId=FGuid::NewGuid();Wrong.DestinationItem.Invalidate();Wrong.Context=FGuid::NewGuid();F.PC->ServerInventoryMoveUI(Wrong);TestEqual(TEXT("Foreign context refused"),F.PC->InventoryUIRevision(),Revision);
    TestTrue(TEXT("Capture before position change"),F.PC->CaptureInventoryDrag(1,Wrong));TestTrue(TEXT("Independent authoritative move"),F.L->Move(F.C->GetInventory(),Original.Id,2));
    TestFalse(TEXT("Stale origin cannot move replacement or current item"),F.PC->SubmitInventoryDrop(Wrong,3));TestEqual(TEXT("All identities conserved"),F.L->GetItemCount(),3);
    F.C->TakeDamage(100,FDamageEvent(),nullptr,nullptr);F.PC->RefreshInventoryContext();TestTrue(TEXT("Terminal result read only"),F.PC->IsInventoryReadOnly());TestFalse(TEXT("Dead cannot start drag"),F.PC->CaptureInventoryDrag(8,Wrong));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6UIInputTest,"Marau6.Inventory.UIInput",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6UIInputTest::RunTest(const FString&)
{
    FUIWorld F;F.PC->SetInventoryNotice(TEXT("Transferencia confirmada."));F.PC->SelectInventorySlot(8);F.PC->HandleInventoryKey(EKeys::H);
    TestTrue(TEXT("Healing refusal replaces earlier transfer success"),F.PC->GetInventoryNotice().Contains(TEXT("Vida cheia")));
    F.C->ConsumeMovementInputVector();F.C->DoMove(1,0);TestTrue(TEXT("Open inventory blocks movement input"),F.C->GetPendingMovementInputVector().IsNearlyZero());F.C->DoJumpStart();TestFalse(TEXT("Open inventory blocks jump"),F.C->bPressedJump);
    F.C->DoAim(20,10);TestTrue(TEXT("Open inventory blocks look input"),F.PC->RotationInput.IsZero());
    TestTrue(TEXT("Mouse/keyboard selection uses same source"),F.PC->HandleInventoryKey(EKeys::Down));F.PC->SelectInventorySlot(8);TestEqual(TEXT("Mouse-selected slot shared"),F.C->GetSelectedInventorySlot(),8);
    F.C->TakeDamage(60,FDamageEvent(),nullptr,nullptr);F.PC->HandleInventoryKey(EKeys::H);TestTrue(TEXT("H uses selected safe supply"),F.C->GetHealing()->IsHealing());TestFalse(TEXT("Healing closes inventory"),F.C->IsInventoryOpen());
    F.C->GetHealing()->ServerCancel();F.C->DoMove(1,0);TestFalse(TEXT("Closing restores movement input"),F.C->GetPendingMovementInputVector().IsNearlyZero());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6UIAckTest,"Marau6.Inventory.UIAcknowledgement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6UIAckTest::RunTest(const FString&)
{
    FUIWorld F;F.PC->PendingInventoryRequest=FGuid::NewGuid();F.PC->PendingInventoryContext=F.PC->GetInventoryContext();F.PC->LastInventoryContext=F.PC->GetInventoryContext();
    const FGuid Pending=F.PC->PendingInventoryRequest;const int32 Expected=F.PC->InventoryUIRevision()+1;
    F.PC->ClientInventoryResult(Pending,F.PC->GetInventoryContext(),true,TEXT("Transferencia confirmada."),Expected);
    TestTrue(TEXT("Ack before state does not unlock next move"),F.PC->InventoryRequestPending());
    F.PC->ClientInventoryResult(FGuid::NewGuid(),F.PC->GetInventoryContext(),true,TEXT("wrong reply"),Expected);TestFalse(TEXT("Unrelated response ignored"),F.PC->GetInventoryNotice().Contains(TEXT("wrong")));
    auto S=F.C->GetInventory()->GetSnapshot();F.C->GetInventory()->ApplySnapshot(S);F.PC->UpdateInventoryUI();
    TestFalse(TEXT("Confirmed state unlocks next operation"),F.PC->InventoryRequestPending());TestTrue(TEXT("Success message after matching state"),F.PC->GetInventoryNotice().Contains(TEXT("confirmada")));
    F.PC->bUseBase=true;F.PC->BaseProfile=1;F.PC->BaseJson=TEXT("{\"items\":[],\"revision\":1}");F.PC->BaseMessage=TEXT("Base pronta");F.PC->UpdateInventoryUI();
    F.PC->InventoryMessage=TEXT("Transferencia salva.");F.PC->BaseMessage=TEXT("Perfil ocupado pelo outro jogador");F.PC->UpdateInventoryUI();
    TestTrue(TEXT("New authoritative base refusal replaces old success notice"),F.PC->GetInventoryNotice().Contains(TEXT("Perfil ocupado")));
    return true;
}
#endif
