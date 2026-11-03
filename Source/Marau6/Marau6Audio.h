#pragma once
#include "CoreMinimal.h"
class UReverbEffect;
struct FSoundAttenuationSettings;

/** Shared mix for the prototype: one soft room reverb and how each sound feeds it. */
namespace Marau6Audio
{
    inline const FName AmbientReverbTag(TEXT("Marau6Ambient"));
    static constexpr float AmbientReverbVolume = .5f;

    // Mix. The Uzi clip and the synthesized rifle report are equally loud at the
    // source (-18 LUFS), so they share one volume and one falloff; together they
    // keep the gunfire level the footsteps were balanced against.
    // Footsteps are ~15 dB quieter at the source than the Uzi, so they sit near full volume.
    static constexpr float ShotOwner = .45f, ShotWorld = .54f;
    static constexpr float ReportOwner = ShotOwner, ReportWorld = ShotWorld;
    static constexpr float GunfireInner = 300.f, GunfireFalloff = 7700.f;
    static constexpr float StepOwner = .5f, StepWorld = 1.f;
    static constexpr float LandVolume = .8f, JumpVolume = .6f;
    // Decompression gas on raid entry, pitched up a little for a tighter hiss.
    static constexpr float GasVolume = .5f, GasPitch = 1.15f;

    /** Short, warm room tail: grounds dry clips without washing out gunfire. */
    void ConfigureAmbientReverb(UReverbEffect& Reverb);

    /** Sounds the owner hears at their own head: no panning or distance, constant send. */
    void ConfigureOwnerSend(FSoundAttenuationSettings& Settings, float Send);

    /** Distant gunfire drops in dB with distance instead of staying loud until the far edge. */
    void ConfigureGunfireFalloff(FSoundAttenuationSettings& Settings, float InnerRadius, float Falloff);

    /** World sounds get wetter with distance, between the attenuation's inner radius and its silence. */
    void ConfigureDistanceSend(FSoundAttenuationSettings& Settings, float NearSend, float FarSend);
}
