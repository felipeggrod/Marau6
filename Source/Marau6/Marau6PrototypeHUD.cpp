#include "Marau6PrototypeHUD.h"
#include "Marau6Character.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

void AMarau6PrototypeHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner) return;
    const AMarau6Character* Character = Cast<AMarau6Character>(PlayerOwner->GetPawn());
    if (!Character) return;

    DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.65f), 12.f, 12.f, 400.f, 150.f);
    const bool bAlive = Character->IsAlive();
    const EMarau6RaidState Raid = Character->GetRaidState();
    DrawText(FString::Printf(TEXT("VIDA: %.0f / 100"), Character->GetHealth()),
        bAlive ? FLinearColor::White : FLinearColor::Red, 22.f, 20.f,
        GEngine->GetMediumFont(), 1.f);
    DrawText(FString::Printf(TEXT("MUNICAO: %d | RESERVA: %d"),
        Character->GetMagazineAmmo(), Character->GetReserveAmmo()),
        FLinearColor::White, 22.f, 50.f, GEngine->GetSmallFont(), 1.f);
    const TCHAR* Status = Raid == EMarau6RaidState::Extracted ? TEXT("EXTRAIDO - reinicie o teste") : !bAlive ? TEXT("ELIMINADO - reinicie o teste")
        : Character->IsReloading() ? TEXT("RECARREGANDO...")
        : Character->GetMagazineAmmo() == 0 && Character->GetReserveAmmo() == 0
            ? TEXT("SEM MUNICAO")
            : TEXT("Clique: disparar | R: recarregar");
    DrawText(Status, FLinearColor::White, 22.f, 78.f, GEngine->GetSmallFont(), 1.f);
    FString ExtractionText = TEXT("EXTRACAO: procure a area verde e permaneca nela");
    if (Raid == EMarau6RaidState::Extracting)
        ExtractionText = FString::Printf(TEXT("EXTRAINDO: %.1f s | sair da area cancela"), Character->GetExtractionSecondsRemaining());
    else if (Raid == EMarau6RaidState::Extracted)
        ExtractionText = TEXT("RESULTADO: EXTRAIDO COM SUCESSO");
    else if (!bAlive)
        ExtractionText = TEXT("RESULTADO: ELIMINADO");
    DrawText(ExtractionText, Raid == EMarau6RaidState::Extracted ? FLinearColor::Green : FLinearColor::White,
        22.f, 112.f, GEngine->GetSmallFont(), 1.f);
    if (!Character->CanParticipate())
    {
        DrawRect(FLinearColor(0.f,0.f,0.f,.7f), Canvas->ClipX*.5f-190.f, Canvas->ClipY*.5f-35.f,380.f,80.f);
        DrawText(Raid == EMarau6RaidState::Extracted ? TEXT("EXTRAIDO COM SUCESSO") : TEXT("ELIMINADO"),
            Raid == EMarau6RaidState::Extracted ? FLinearColor::Green : FLinearColor::Red,
            Canvas->ClipX*.5f-165.f, Canvas->ClipY*.5f-22.f, GEngine->GetMediumFont(),1.f);
        DrawText(TEXT("Reinicie o Play para uma nova partida de teste"),FLinearColor::White,
            Canvas->ClipX*.5f-175.f, Canvas->ClipY*.5f+12.f,GEngine->GetSmallFont(),1.f);
    }
    if (Character->CanParticipate())
    {
        const float X = Canvas->ClipX * 0.5f;
        const float Y = Canvas->ClipY * 0.5f;
        DrawLine(X - 7.f, Y, X + 7.f, Y, FLinearColor::White, 1.5f);
        DrawLine(X, Y - 7.f, X, Y + 7.f, FLinearColor::White, 1.5f);
    }
}

