#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Marau6Character.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6ShotEffectsTest, "Marau6.Combat.ShotEffects",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMarau6ShotEffectsTest::RunTest(const FString& Parameters)
{
    UClass* EffectClass = LoadClass<AActor>(nullptr, TEXT("/Script/Marau6.Marau6ShotEffect"));
    if (!TestNotNull(TEXT("Cosmetic shot effect exists"), EffectClass)) return false;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    ON_SCOPE_EXIT { GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
        if (World->IsRooted()) World->RemoveFromRoot(); };
    UClass* PawnClass = LoadClass<AMarau6Character>(nullptr,
        TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C"));
    FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Shooter = World->SpawnActor<AMarau6Character>(PawnClass, FVector(0,0,200), FRotator::ZeroRotator, Spawn);
    auto* Target = World->SpawnActor<AMarau6Character>(PawnClass, FVector(500,0,200), FRotator::ZeroRotator, Spawn);
    auto* PC = World->SpawnActor<APlayerController>();
    if (!Shooter || !Target || !PC) return false;
    PC->Possess(Shooter); PC->SetControlRotation(FRotator::ZeroRotator);
    auto Effects = [&]() { TArray<AActor*> Items; for (TActorIterator<AActor> It(World, EffectClass); It; ++It) Items.Add(*It); return Items; };
    auto Fire = [&]() { Shooter->ProcessEvent(Shooter->FindFunctionChecked(TEXT("ServerFire")), nullptr); };
    ++GFrameCounter; World->GetTimerManager().Tick(0.f);
    Fire(); auto Items = Effects();
    if (!TestEqual(TEXT("Accepted shot creates exactly one local effect"), Items.Num(), 1)) return false;
    AActor* Effect = Items[0];
    TestFalse(TEXT("Cosmetic actor does not replicate"), Effect->GetIsReplicated());
    TestTrue(TEXT("Effect has bounded lifespan"), Effect->GetLifeSpan() > 0 && Effect->GetLifeSpan() < 1);
    auto* Flash = FindObject<UStaticMeshComponent>(Effect, TEXT("MuzzleFlash"));
    auto* Impact = FindObject<UStaticMeshComponent>(Effect, TEXT("Impact"));
    if (!TestNotNull(TEXT("Flash component"), Flash) || !TestNotNull(TEXT("Impact component"), Impact)) return false;
    TestNotNull(TEXT("Flash mesh"), Flash->GetStaticMesh().Get());
    TestNotNull(TEXT("Flash material"), Flash->GetMaterial(0));
    TestEqual(TEXT("Flash at muzzle socket"), Flash->GetAttachSocketName(), FName("Muzzle"));
    TestTrue(TEXT("Blocking hit shows impact"), Impact->IsVisible());
    TestEqual(TEXT("Flash no collision"), Flash->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    TestEqual(TEXT("Impact no collision"), Impact->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    Fire(); TestEqual(TEXT("Cooldown rejection creates no effect"), Effects().Num(), 1);
    ++GFrameCounter; World->Tick(LEVELTICK_TimeOnly, .3f); World->GetTimerManager().Tick(.3f);
    TestEqual(TEXT("Transient effects are cleaned up"), Effects().Num(), 0);
    PC->SetControlRotation(FRotator(0,90,0)); Fire(); Items = Effects();
    if (!TestEqual(TEXT("Air shot creates flash"), Items.Num(), 1)) return false;
    Impact = FindObject<UStaticMeshComponent>(Items[0], TEXT("Impact"));
    TestFalse(TEXT("Air shot has no fake impact"), Impact->IsVisible());
    ++GFrameCounter; World->Tick(LEVELTICK_TimeOnly, .3f); World->GetTimerManager().Tick(.3f);
    Shooter->ProcessEvent(Shooter->FindFunctionChecked(TEXT("ServerReload")), nullptr);
    Fire(); TestEqual(TEXT("Reload rejection creates no effect"), Effects().Num(), 0);
    FDamageEvent Damage; Shooter->TakeDamage(100, Damage, PC, Target);
    Fire(); TestEqual(TEXT("Dead shooter creates no effect"), Effects().Num(), 0);
    return true;
}
#endif
