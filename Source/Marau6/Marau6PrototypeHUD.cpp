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

    DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.65f), 12.f, 12.f, 300.f, 80.f);
    const bool bAlive = Character->IsAlive();
    DrawText(FString::Printf(TEXT("VIDA: %.0f / 100"), Character->GetHealth()),
        bAlive ? FLinearColor::White : FLinearColor::Red, 22.f, 20.f,
        GEngine->GetMediumFont(), 1.f);
    DrawText(bAlive ? TEXT("Clique esquerdo: disparar") : TEXT("ELIMINADO - reinicie o teste"),
        FLinearColor::White, 22.f, 50.f, GEngine->GetSmallFont(), 1.f);
    if (bAlive)
    {
        const float X = Canvas->ClipX * 0.5f;
        const float Y = Canvas->ClipY * 0.5f;
        DrawLine(X - 7.f, Y, X + 7.f, Y, FLinearColor::White, 1.5f);
        DrawLine(X, Y - 7.f, X, Y + 7.f, FLinearColor::White, 1.5f);
    }
}

