#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Marau6Character.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/UnrealType.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6HealthTest, "Marau6.Combat.Health",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMarau6HealthTest::RunTest(const FString& Parameters)
{
    UClass* CharacterClass = LoadClass<AMarau6Character>(nullptr,
        TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C"));
    if (!TestNotNull(TEXT("First person character class"), CharacterClass)) return false;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AMarau6Character* Character = World->SpawnActor<AMarau6Character>(CharacterClass);
    if (!TestNotNull(TEXT("Character spawned"), Character)) { World->DestroyWorld(false); return false; }
    const FFloatProperty* HealthProperty = FindFProperty<FFloatProperty>(AMarau6Character::StaticClass(), TEXT("Health"));
    if (!TestNotNull(TEXT("Health property"), HealthProperty)) { World->DestroyWorld(false); return false; }
    auto Health = [&]() { return HealthProperty->GetPropertyValue_InContainer(Character); };
    Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    FDamageEvent Event;
    TestTrue(TEXT("Authority"), Character->HasAuthority());
    TestEqual(TEXT("Initial health"), Health(), 100.f);
    TestEqual(TEXT("Negative damage rejected"), Character->TakeDamage(-25.f, Event, nullptr, nullptr), 0.f);
    TestEqual(TEXT("NaN damage rejected"), Character->TakeDamage(std::numeric_limits<float>::quiet_NaN(), Event, nullptr, nullptr), 0.f);
    TestEqual(TEXT("Infinity rejected"), Character->TakeDamage(std::numeric_limits<float>::infinity(), Event, nullptr, nullptr), 0.f);
    TestEqual(TEXT("Invalid damage does not change health"), Health(), 100.f);
    Character->TakeDamage(25.f, Event, nullptr, nullptr);
    TestEqual(TEXT("First hit"), Health(), 75.f);
    Character->TakeDamage(25.f, Event, nullptr, nullptr);
    TestEqual(TEXT("Second hit"), Health(), 50.f);
    Character->TakeDamage(25.f, Event, nullptr, nullptr);
    TestEqual(TEXT("Third hit"), Health(), 25.f);
    TestEqual(TEXT("Overkill returns actual damage"), Character->TakeDamage(100.f, Event, nullptr, nullptr), 25.f);
    TestEqual(TEXT("Health clamped at zero"), Health(), 0.f);
    TestEqual(TEXT("Movement disabled on death"), Character->GetCharacterMovement()->MovementMode, MOVE_None);
    TestEqual(TEXT("Dead character ignores damage"), Character->TakeDamage(25.f, Event, nullptr, nullptr), 0.f);
    World->DestroyWorld(false);
    return true;
}
#endif




