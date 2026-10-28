#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarau6ShotAudioTest, "Marau6.Combat.ShotAudio",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMarau6ShotAudioTest::RunTest(const FString& Parameters)
{
    UClass* Class = LoadClass<AActor>(nullptr, TEXT("/Script/Marau6.Marau6ShotEffect"));
    if (!TestNotNull(TEXT("Shot effect class"), Class)) return false;
    const auto* SoundProperty = FindFProperty<FObjectPropertyBase>(Class, TEXT("ShotSound"));
    const auto* AttenuationProperty = FindFProperty<FObjectPropertyBase>(Class, TEXT("ShotAttenuation"));
    if (!TestNotNull(TEXT("Shot audio referenced by effect"), SoundProperty)
        || !TestNotNull(TEXT("Remote sound attenuation configured"), AttenuationProperty)) return false;
    const UObject* Defaults = Class->GetDefaultObject();
    USoundBase* Sound = Cast<USoundBase>(SoundProperty->GetObjectPropertyValue_InContainer(Defaults));
    USoundAttenuation* Attenuation = Cast<USoundAttenuation>(AttenuationProperty->GetObjectPropertyValue_InContainer(Defaults));
    if (!TestNotNull(TEXT("Sound asset loaded"), Sound) || !TestNotNull(TEXT("Attenuation object"), Attenuation)) return false;
    TestTrue(TEXT("Shot has a short finite duration"), Sound->GetDuration() > 0.f && Sound->GetDuration() < 5.f);
    TestFalse(TEXT("Shot does not loop"), Sound->IsLooping());
    TestTrue(TEXT("Other player's sound is spatialized"), !!Attenuation->Attenuation.bSpatialize);
    TestTrue(TEXT("Distance attenuation enabled"), !!Attenuation->Attenuation.bAttenuate);
    const FTransform Origin = FTransform::Identity;
    const float Near = Attenuation->Attenuation.Evaluate(Origin, FVector(100,0,0));
    const float Far = Attenuation->Attenuation.Evaluate(Origin, FVector(6000,0,0));
    TestTrue(TEXT("Distant rifle quieter than nearby rifle"), Near > Far && Far > 0.f);
    TestTrue(TEXT("Beyond range sound inaudible"), Attenuation->Attenuation.Evaluate(Origin, FVector(9000,0,0)) <= KINDA_SMALL_NUMBER);
    return true;
}
#endif
