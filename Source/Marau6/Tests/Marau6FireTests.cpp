#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Marau6Character.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/DamageEvents.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6FireTest, "Marau6.Combat.ServerFire",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMarau6FireTest::RunTest(const FString& Parameters)
{
    UFunction* Fire = AMarau6Character::StaticClass()->FindFunctionByName(TEXT("ServerFire"));
    if (!TestNotNull(TEXT("Server fire RPC exists"), Fire)) return false;
    TestTrue(TEXT("Fire is a server RPC"), Fire->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
    TestEqual(TEXT("Client cannot supply shot parameters"), static_cast<int32>(Fire->NumParms), 0);
    UClass* CharacterClass = LoadClass<AMarau6Character>(nullptr,
        TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C"));
    if (!TestNotNull(TEXT("First person class exists"), CharacterClass)) return false;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto MakeCharacter = [&](FVector Position) {
        return World->SpawnActor<AMarau6Character>(CharacterClass, Position, FRotator::ZeroRotator, Spawn);
    };
    AMarau6Character* Shooter = MakeCharacter(FVector(0, 0, 200));
    AMarau6Character* Target = MakeCharacter(FVector(500, 0, 200));
    APlayerController* Controller = World->SpawnActor<APlayerController>();
    if (!Shooter || !Target || !Controller)
    {
        AddError(TEXT("Failed to spawn test actors"));
        GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
        return false;
    }
    Controller->Possess(Shooter);
    Controller->SetControlRotation(FRotator::ZeroRotator);
    TestTrue(TEXT("Shooter is possessed"), Shooter->GetController() == Controller);
    AddInfo(FString::Printf(TEXT("Eye=%s target=%s registered=%d physics=%d collision=%d pawnResponse=%d"),
        *Shooter->GetPawnViewLocation().ToString(), *Target->GetActorLocation().ToString(),
        Target->GetCapsuleComponent()->IsRegistered(), Target->GetCapsuleComponent()->IsPhysicsStateCreated(),
        static_cast<int32>(Target->GetCapsuleComponent()->GetCollisionEnabled()),
        static_cast<int32>(Target->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn))));
    auto Shoot = [&]() { Shooter->ProcessEvent(Fire, nullptr); };
    auto Advance = [&]() { World->Tick(LEVELTICK_TimeOnly, 0.3f); };
    Shoot();
    TestEqual(TEXT("Aimed shot deals 25 damage"), Target->GetHealth(), 75.f);
    Shoot();
    TestEqual(TEXT("Immediate repeated request is rejected"), Target->GetHealth(), 75.f);
    Advance();
    Controller->SetControlRotation(FRotator(0, 90, 0));
    Shoot();
    TestEqual(TEXT("Miss does not damage target"), Target->GetHealth(), 75.f);
    Advance();
    Controller->SetControlRotation(FRotator::ZeroRotator);
    AActor* Wall = World->SpawnActor<AActor>();
    UBoxComponent* WallCollision = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(WallCollision);
    WallCollision->SetBoxExtent(FVector(10, 150, 150));
    WallCollision->SetCollisionProfileName(TEXT("BlockAll"));
    WallCollision->RegisterComponent();
    Wall->SetActorLocation(FVector(250, 0, 200));
    Shoot();
    TestEqual(TEXT("Wall blocks damage"), Target->GetHealth(), 75.f);
    Wall->Destroy();
    Advance();
    Shoot();
    TestEqual(TEXT("Second valid hit"), Target->GetHealth(), 50.f);
    Advance();
    Shoot();
    TestEqual(TEXT("Third valid hit"), Target->GetHealth(), 25.f);
    Advance();
    Shoot();
    TestEqual(TEXT("Fourth valid hit kills target"), Target->GetHealth(), 0.f);
    Target->Destroy();
    AMarau6Character* FarTarget = MakeCharacter(FVector(11000, 0, 200));
    Advance(); Shoot();
    TestEqual(TEXT("Target beyond 100 meters is not hit"), FarTarget->GetHealth(), 100.f);
    FarTarget->Destroy();
    AMarau6Character* NextTarget = MakeCharacter(FVector(500, 0, 200));
    FDamageEvent Damage;
    Shooter->TakeDamage(100.f, Damage, nullptr, nullptr);
    Advance();
    Shoot();
    TestEqual(TEXT("Dead shooter cannot deal damage"), NextTarget->GetHealth(), 100.f);
    GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
    return true;
}
#endif


