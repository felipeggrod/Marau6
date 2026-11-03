#pragma once
#include "CoreMinimal.h"
class UReverbEffect;
struct FSoundAttenuationSettings;

/** Shared mix for the prototype: one soft room reverb and how each sound feeds it. */
namespace Marau6Audio
{
    inline const FName AmbientReverbTag(TEXT("Marau6Ambient"));
    static constexpr float AmbientReverbVolume = .5f;

    /** Short, warm room tail: grounds dry clips without washing out gunfire. */
    void ConfigureAmbientReverb(UReverbEffect& Reverb);

    /** Sounds the owner hears at their own head: no panning or distance, constant send. */
    void ConfigureOwnerSend(FSoundAttenuationSettings& Settings, float Send);

    /** World sounds get wetter with distance, between the attenuation's inner radius and its silence. */
    void ConfigureDistanceSend(FSoundAttenuationSettings& Settings, float NearSend, float FarSend);
}
