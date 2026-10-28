#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Marau6Character.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6AmmoTest, "Marau6.Combat.AmmoReload",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMarau6AmmoTest::RunTest(const FString& Parameters)
{
    UFunction* Reload = AMarau6Character::StaticClass()->FindFunctionByName(TEXT("ServerReload"));
    if (!TestNotNull(TEXT("Server reload RPC exists"), Reload)) return false;
    UFunction* Fire = AMarau6Character::StaticClass()->FindFunctionByName(TEXT("ServerFire"));
    const FStructProperty* State = FindFProperty<FStructProperty>(AMarau6Character::StaticClass(), TEXT("AmmoState"));
    if (!TestNotNull(TEXT("Ammo state exists"), State) || !TestNotNull(TEXT("Fire RPC exists"), Fire)) return false;
    const FIntProperty* Magazine = FindFProperty<FIntProperty>(State->Struct, TEXT("Magazine"));
    const FIntProperty* Reserve = FindFProperty<FIntProperty>(State->Struct, TEXT("Reserve"));
    const FBoolProperty* Reloading = FindFProperty<FBoolProperty>(State->Struct, TEXT("bReloading"));
    if (!TestNotNull(TEXT("Magazine field"), Magazine) || !TestNotNull(TEXT("Reserve field"), Reserve)
        || !TestNotNull(TEXT("Reload flag"), Reloading)) return false;
    TestTrue(TEXT("Reload RPC is server-only"), Reload->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
    TestEqual(TEXT("Reload RPC accepts no ammo values"), static_cast<int32>(Reload->NumParms), 0);
    TestTrue(TEXT("Ammo state is replicated"), State->HasAnyPropertyFlags(CPF_Net));

    UClass* Class = LoadClass<AMarau6Character>(nullptr,
        TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C"));
    if (!TestNotNull(TEXT("Character class"), Class)) return false;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    ON_SCOPE_EXIT
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        if (World->IsRooted()) World->RemoveFromRoot();
    };
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto MakeCharacter = [&](FVector Position) {
        return World->SpawnActor<AMarau6Character>(Class, Position, FRotator::ZeroRotator, Spawn);
    };
    AMarau6Character* Shooter = MakeCharacter(FVector(0, 0, 200));
    APlayerController* Controller = World->SpawnActor<APlayerController>();
    if (!TestNotNull(TEXT("Shooter"), Shooter) || !TestNotNull(TEXT("Controller"), Controller)) return false;
    Controller->Possess(Shooter);
    Controller->SetControlRotation(FRotator(0, 90, 0));
    auto Mag = [&]() { return Magazine->GetPropertyValue_InContainer(State->ContainerPtrToValuePtr<void>(Shooter)); };
    auto Res = [&]() { return Reserve->GetPropertyValue_InContainer(State->ContainerPtrToValuePtr<void>(Shooter)); };
    auto Busy = [&]() { return Reloading->GetPropertyValue_InContainer(State->ContainerPtrToValuePtr<void>(Shooter)); };
    // Drive separate synthetic frames so the real TimerManager advances once per step.
    auto Advance = [&](float Seconds) {
        ++GFrameCounter;
        World->Tick(LEVELTICK_TimeOnly, Seconds);
        World->GetTimerManager().Tick(Seconds);
    };
    auto Shoot = [&]() { Shooter->ProcessEvent(Fire, nullptr); };
    auto StartReload = [&]() { Shooter->ProcessEvent(Reload, nullptr); };
    auto Empty = [&](int32 Rounds) { for (int32 I=0; I<Rounds; ++I) { Shoot(); Advance(0.3f); } };
    Advance(0.f);
    TestEqual(TEXT("Initial magazine"), Mag(), 10);
    TestEqual(TEXT("Initial reserve"), Res(), 30);
    StartReload();
    TestFalse(TEXT("Full magazine ignores reload"), Busy());
    Shoot();
    TestEqual(TEXT("Accepted miss spends one round"), Mag(), 9);
    Shoot();
    TestEqual(TEXT("Rate-limited request spends nothing"), Mag(), 9);
    StartReload();
    TestTrue(TEXT("Reload starts"), Busy());
    Shoot();
    TestEqual(TEXT("Cannot fire while reloading"), Mag(), 9);
    Advance(1.f);
    TestTrue(TEXT("Reload still in progress after one second"), Busy());
    TestEqual(TEXT("No early transfer"), Res(), 30);
    StartReload();
    Advance(1.05f);
    TestFalse(TEXT("Repeated request does not restart reload"), Busy());
    TestEqual(TEXT("Partial reload fills only one round"), Mag(), 10);
    TestEqual(TEXT("Partial reload consumes only one reserve round"), Res(), 29);
    Empty(10);
    TestEqual(TEXT("Magazine empty after ten shots"), Mag(), 0);
    AMarau6Character* Target = MakeCharacter(FVector(500, 0, 200));
    if (!TestNotNull(TEXT("Target"), Target)) return false;
    Controller->SetControlRotation(FRotator::ZeroRotator);
    Shoot();
    TestEqual(TEXT("Empty gun does no damage"), Target->GetHealth(), 100.f);
    StartReload();
    Shoot();
    TestEqual(TEXT("Reloading gun does no damage"), Target->GetHealth(), 100.f);
    Advance(2.05f);
    TestEqual(TEXT("Empty reload fills ten"), Mag(), 10);
    TestEqual(TEXT("Reserve reduced by ten"), Res(), 19);
    Controller->SetControlRotation(FRotator(0, 90, 0));
    Empty(10); StartReload(); Advance(2.05f);
    TestEqual(TEXT("Next full reload reserve"), Res(), 9);
    Empty(10); StartReload(); Advance(2.05f);
    TestEqual(TEXT("Final partial magazine"), Mag(), 9);
    TestEqual(TEXT("Reserve never goes negative"), Res(), 0);
    Empty(9); StartReload(); Advance(2.05f);
    TestEqual(TEXT("No rounds created with empty reserve"), Mag(), 0);
    TestFalse(TEXT("Cannot reload without reserve"), Busy());

    // Fresh actor proves death cancels a genuinely active reload with reserve available.
    AMarau6Character* Dying = MakeCharacter(FVector(0, 1000, 200));
    if (!TestNotNull(TEXT("Second shooter"), Dying)) return false;
    Controller->Possess(Dying);
    Controller->SetControlRotation(FRotator(0, 90, 0));
    Dying->ProcessEvent(Fire, nullptr);
    Dying->ProcessEvent(Reload, nullptr);
    void* DyingState = State->ContainerPtrToValuePtr<void>(Dying);
    TestTrue(TEXT("Second shooter starts reloading"), Reloading->GetPropertyValue_InContainer(DyingState));
    FDamageEvent Damage;
    Dying->TakeDamage(100.f, Damage, nullptr, nullptr);
    Advance(2.1f);
    TestFalse(TEXT("Death clears reload state"), Reloading->GetPropertyValue_InContainer(DyingState));
    TestEqual(TEXT("Death prevents ammo transfer"), Magazine->GetPropertyValue_InContainer(DyingState), 9);
    TestEqual(TEXT("Death preserves reserve"), Reserve->GetPropertyValue_InContainer(DyingState), 30);
    Dying->ProcessEvent(Reload, nullptr);
    TestFalse(TEXT("Dead character cannot start reload"), Reloading->GetPropertyValue_InContainer(DyingState));
    return true;
}
#endif
