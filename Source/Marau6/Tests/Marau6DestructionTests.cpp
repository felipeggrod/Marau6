#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include <limits>
#include "Engine/World.h"
#include "Marau6DestructibleSection.h"
#include "Marau6Destruction.h"
#include "Marau6Character.h"
#include "Marau6InventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavMesh/RecastNavMesh.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/DamageEvents.h"
#include "UObject/UnrealType.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6SectionDamageTest,"Marau6.Destruction.SectionDamage",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6SectionDamageTest::RunTest(const FString&)
{
    UClass* Class=FindObject<UClass>(nullptr,TEXT("/Script/Marau6.Marau6DestructibleSection"));
    if(!TestNotNull(TEXT("Server-authoritative destructible section exists"),Class))return false;
    auto* Health=FindFProperty<FFloatProperty>(Class,TEXT("Health"));if(!TestNotNull(TEXT("Panel health replicated"),Health))return false;
    TestTrue(TEXT("Health is replicated"),Health->HasAnyPropertyFlags(CPF_Net));
    UWorld* W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);W->InitializeActorsForPlay(FURL());
    AActor* A=W->SpawnActor<AActor>(Class,FVector(0,0,120),FRotator::ZeroRotator);AActor* B=W->SpawnActor<AActor>(Class,FVector(0,120,120),FRotator::ZeroRotator);
    if(A && B)
    {
        TestEqual(TEXT("Initial200HP"),Health->GetPropertyValue_InContainer(A),200.f);
        for(int32 N=0;N<7;N++)A->TakeDamage(25,FDamageEvent(),nullptr,nullptr);
        TestEqual(TEXT("Seven hits leave25HP"),Health->GetPropertyValue_InContainer(A),25.f);TestEqual(TEXT("Neighbor unchanged"),Health->GetPropertyValue_InContainer(B),200.f);
        A->TakeDamage(1000,FDamageEvent(),nullptr,nullptr);TestEqual(TEXT("Generic damage clamped at zero"),Health->GetPropertyValue_InContainer(A),0.f);
        TestEqual(TEXT("Broken receives no more damage"),A->TakeDamage(25,FDamageEvent(),nullptr,nullptr),0.f);
        B->TakeDamage(-1,FDamageEvent(),nullptr,nullptr);B->TakeDamage(std::numeric_limits<float>::infinity(),FDamageEvent(),nullptr,nullptr);TestEqual(TEXT("Invalid damage ignored"),Health->GetPropertyValue_InContainer(B),200.f);
    }
    GEngine->DestroyWorldContext(W);W->DestroyWorld(false);if(W->IsRooted())W->RemoveFromRoot();return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6SectionLifecycleTest,"Marau6.Destruction.Lifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6SectionLifecycleTest::RunTest(const FString&)
{
    TestNotNull(TEXT("Readiness must track pending navigation after reset"),FindFProperty<FBoolProperty>(UMarau6Destruction::StaticClass(),TEXT("bNavigationReady")));
    TestEqual(TEXT("Nav runtime accepts dynamic obstruction changes"),GetDefault<ARecastNavMesh>()->GetRuntimeGenerationMode(),ERuntimeGenerationType::Dynamic);
    UWorld* W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);W->InitializeActorsForPlay(FURL());
    ON_SCOPE_EXIT{GEngine->DestroyWorldContext(W);W->DestroyWorld(false);if(W->IsRooted())W->RemoveFromRoot();};
    auto* A=W->SpawnActor<AMarau6DestructibleSection>(FVector(0,0,120),FRotator::ZeroRotator);auto* Manager=W->GetSubsystem<UMarau6Destruction>();
    Manager->ResetForRound(TEXT("first"));TestFalse(TEXT("Round reset waits for nav work"),Manager->IsReadyForRound());A->TakeDamage(25,FDamageEvent(),nullptr,nullptr);Manager->ResetForRound(TEXT("first"));TestEqual(TEXT("Same round does not repair"),A->GetHealth(),175.f);
    A->TakeDamage(200,FDamageEvent(),nullptr,nullptr);TestTrue(TEXT("Broken before next round"),A->IsBroken());Manager->ResetForRound(TEXT("second"));TestEqual(TEXT("New round restores200"),A->GetHealth(),200.f);
    A->TakeDamage(25,FDamageEvent(),nullptr,nullptr);A->SetRole(ROLE_SimulatedProxy);TestEqual(TEXT("Client damage ignored"),A->TakeDamage(100,FDamageEvent(),nullptr,nullptr),0.f);A->ResetSection();TestEqual(TEXT("Client cannot reset health"),A->GetHealth(),175.f);A->SetRole(ROLE_Authority);A->ResetSection();
    FHitResult Hit;TestTrue(TEXT("Restored section blocks visibility"),W->LineTraceSingleByChannel(Hit,FVector(-200,0,120),FVector(200,0,120),ECC_Visibility));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6SectionFireTest,"Marau6.Destruction.ServerFire",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarau6SectionFireTest::RunTest(const FString&)
{
    UWorld* W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);W->InitializeActorsForPlay(FURL());
    ON_SCOPE_EXIT{GEngine->DestroyWorldContext(W);W->DestroyWorld(false);if(W->IsRooted())W->RemoveFromRoot();};
    auto* C=LoadClass<AMarau6Character>(nullptr,TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C"));FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Shooter=W->SpawnActor<AMarau6Character>(C,FVector(-400,0,96),FRotator::ZeroRotator,Spawn);auto* PC=W->SpawnActor<APlayerController>();PC->Possess(Shooter);PC->SetControlRotation(FRotator::ZeroRotator);Shooter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    auto* Target=W->SpawnActor<AMarau6Character>(C,FVector(400,0,96),FRotator::ZeroRotator,Spawn);
    auto* Section=W->SpawnActor<AMarau6DestructibleSection>(FVector(0,0,120),FRotator::ZeroRotator);auto* Neighbor=W->SpawnActor<AMarau6DestructibleSection>(FVector(0,120,120),FRotator::ZeroRotator);
    for(int32 N=0;N<8;N++){Shooter->ProcessEvent(Shooter->FindFunction(TEXT("ServerFire")),nullptr);++GFrameCounter;W->Tick(LEVELTICK_TimeOnly,.3f);}
    TestTrue(TEXT("Eight rifle hits break exact panel"),Section->IsBroken());TestEqual(TEXT("Neighbor remains intact"),Neighbor->GetHealth(),200.f);TestEqual(TEXT("Breaking shot stops at wall"),Target->GetHealth(),100.f);
    TestEqual(TEXT("Eight shots cost8 rounds"),Shooter->GetMagazineAmmo(),12);TestEqual(TEXT("Eight shots cost8 percent"),Shooter->GetRifleDurability(),9200);
    Shooter->ProcessEvent(Shooter->FindFunction(TEXT("ServerFire")),nullptr);TestEqual(TEXT("Next shot passes through opening"),Target->GetHealth(),75.f);
    FHitResult Hit;TestFalse(TEXT("Opening permits capsule passage"),W->SweepSingleByChannel(Hit,FVector(-100,0,96),FVector(100,0,96),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,90)));
    return true;
}
#endif
