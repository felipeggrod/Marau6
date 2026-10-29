#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Marau6Character.h"
#include "Marau6ExtractionZone.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6ExtractionTest, "Marau6.Match.Extraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMarau6ExtractionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    ON_SCOPE_EXIT { GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
        if (World->IsRooted()) World->RemoveFromRoot(); };
    auto* Zone = World->SpawnActor<AMarau6ExtractionZone>();
    UClass* Class = LoadClass<AMarau6Character>(nullptr,
        TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C"));
    FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto MakePawn = [&]() {
        auto* Pawn = World->SpawnActor<AMarau6Character>(Class, FVector(1000,0,96), FRotator::ZeroRotator, Spawn);
        auto* PC = World->SpawnActor<APlayerController>(); PC->Possess(Pawn);
        Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        Pawn->DispatchBeginPlay();
        TestTrue(TEXT("Runtime character tick enabled"), Pawn->IsActorTickEnabled());
        return Pawn;
    };
    auto Advance = [&](float Seconds) {
        while (Seconds > KINDA_SMALL_NUMBER) {
            const float Step = FMath::Min(.1f, Seconds);
            ++GFrameCounter; World->Tick(LEVELTICK_TimeOnly, Step);
            World->GetTimerManager().Tick(Step); Seconds -= Step;
        }
    };
    auto Update = [&](AMarau6Character* Pawn) { Pawn->Tick(0.f); };
    auto* A = MakePawn(); auto* B = MakePawn();
    TestEqual(TEXT("Player starts active"), A->GetRaidState(), EMarau6RaidState::Active);
    TestFalse(TEXT("Outside not contained"), Zone->ContainsCharacter(A));
    A->SetActorLocation(FVector(0,0,96)); Update(A);
    TestEqual(TEXT("Entering starts extraction"), A->GetRaidState(), EMarau6RaidState::Extracting);
    TestTrue(TEXT("Countdown starts at five seconds"), FMath::IsNearlyEqual(A->GetExtractionSecondsRemaining(),5.f,.05f));
    Advance(4.f); Update(A);
    TestEqual(TEXT("No early extraction"), A->GetRaidState(), EMarau6RaidState::Extracting);
    A->SetActorLocation(FVector(1000,0,96)); Update(A);
    TestEqual(TEXT("Exit cancels countdown"), A->GetRaidState(), EMarau6RaidState::Active);
    A->SetActorLocation(FVector(0,0,96)); Update(A); Advance(2.f); Update(A);
    TestEqual(TEXT("Reentry does not reuse old elapsed time"), A->GetRaidState(), EMarau6RaidState::Extracting);
    TestEqual(TEXT("Other player unaffected"), B->GetRaidState(), EMarau6RaidState::Active);
    A->ProcessEvent(A->FindFunctionChecked(TEXT("ServerFire")),nullptr);
    TestEqual(TEXT("Can shoot during countdown"), A->GetMagazineAmmo(),9);
    Advance(2.5f); Update(A);
    A->ProcessEvent(A->FindFunctionChecked(TEXT("ServerReload")),nullptr);
    TestTrue(TEXT("Can reload during countdown"), A->IsReloading());
    Advance(.6f); Update(A);
    TestEqual(TEXT("Continuous presence completes extraction"), A->GetRaidState(), EMarau6RaidState::Extracted);
    TestFalse(TEXT("Extracted cannot participate"), A->CanParticipate());
    TestFalse(TEXT("Extraction cancels reload"), A->IsReloading());
    TestTrue(TEXT("Extracted hidden from combat world"), A->IsHidden());
    TestFalse(TEXT("Extracted has no collision"), A->GetActorEnableCollision());
    TestEqual(TEXT("Extracted movement disabled"), A->GetCharacterMovement()->MovementMode.GetValue(), MOVE_None);
    const int32 Rounds = A->GetMagazineAmmo();
    A->ProcessEvent(A->FindFunctionChecked(TEXT("ServerFire")),nullptr);
    A->ProcessEvent(A->FindFunctionChecked(TEXT("ServerReload")),nullptr);
    TestEqual(TEXT("Extracted cannot spend ammo"), A->GetMagazineAmmo(),Rounds);
    TestFalse(TEXT("Extracted cannot restart reload"), A->IsReloading());
    FDamageEvent Damage;
    TestEqual(TEXT("Extracted cannot take damage"), A->TakeDamage(25,Damage,B->GetController(),B),0.f);
    TestEqual(TEXT("Extraction preserves health"), A->GetHealth(),100.f);
    Advance(2.f);
    TestEqual(TEXT("Cancelled reload never transfers ammo"), A->GetMagazineAmmo(),9);
    TestEqual(TEXT("Cancelled reload preserves reserve"), A->GetReserveAmmo(),30);
    B->SetActorLocation(FVector(0,0,96)); Update(B); Advance(4.f);
    B->TakeDamage(100,Damage,B->GetController(),A); Update(B); Advance(3.f); Update(B);
    TestEqual(TEXT("Death wins over countdown"), B->GetRaidState(), EMarau6RaidState::Eliminated);
    TestEqual(TEXT("Extracted result remains terminal"), A->GetRaidState(), EMarau6RaidState::Extracted);
    auto* C = MakePawn(); C->SetActorLocation(FVector(0,0,96)); Update(C);
    Zone->Destroy(); Update(C);
    TestEqual(TEXT("Destroyed zone cancels safely"), C->GetRaidState(), EMarau6RaidState::Active);
    TestNull(TEXT("No client-callable extraction completion"), A->FindFunction(TEXT("CompleteExtraction")));
    return true;
}
#endif
