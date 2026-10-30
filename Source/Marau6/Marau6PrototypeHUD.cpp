#include "Marau6PrototypeHUD.h"
#include "Marau6Character.h"
#include "Marau6InventoryComponent.h"
#include "Marau6LootActor.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

void AMarau6PrototypeHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner) return;
    const AMarau6Character* Character = Cast<AMarau6Character>(PlayerOwner->GetPawn());
    if (!Character) return;

    DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.65f), 12.f, 12.f, 400.f, 180.f);
    const bool bAlive = Character->IsAlive();
    const EMarau6RaidState Raid = Character->GetRaidState();
    DrawText(FString::Printf(TEXT("VIDA: %.0f / 100"), Character->GetHealth()),
        bAlive ? FLinearColor::White : FLinearColor::Red, 22.f, 20.f,
        GEngine->GetMediumFont(), 1.f);
    DrawText(FString::Printf(TEXT("MUNICAO: %d | RESERVA: %d"),
        Character->GetMagazineAmmo(), Character->GetReserveAmmo()),
        FLinearColor::White, 22.f, 50.f, GEngine->GetSmallFont(), 1.f);
    const TCHAR* Status = Raid == EMarau6RaidState::Extracted ? TEXT("EXTRAIDO - reinicie o teste") : !bAlive ? TEXT("ELIMINADO - reinicie o teste")
        : !Character->HasEquippedRifle() ? TEXT("SEM ARMA - equipe um rifle pelo inventario")
        : Character->IsInventoryOpen() ? TEXT("TAB: fechar inventario | R: recarregar")
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
    const UMarau6InventoryComponent* Inventory=Character->GetInventory();
    const auto& Snapshot=Inventory->GetSnapshot();
    DrawText(TEXT("TAB: inventario | E: coletar loot sob a mira"),FLinearColor::White,22,140,GEngine->GetSmallFont(),1);
    if(Snapshot.bFinalized)
        DrawText(FString::Printf(TEXT("ITENS MANTIDOS: %d | PERDIDOS: %d"),Snapshot.Items.Num(),Snapshot.LostCount),
            FLinearColor::White,22,163,GEngine->GetSmallFont(),1);
    if(Character->CanParticipate())
    {
        if(const auto* Loot=Inventory->LookedAtLoot())
        {
            bool bHasSpace=Loot->GetItem().IsRifle() && !Character->HasEquippedRifle();
            for(int32 Slot=0;Slot<Marau6Slots::BackpackCount;++Slot) if(!Inventory->ItemAt(Slot)) bHasSpace=true;
            const FString Prompt=bHasSpace?TEXT("E: coletar ")+Loot->GetItem().Name():TEXT("MOCHILA CHEIA");
            DrawText(Prompt,FLinearColor::Yellow,Canvas->ClipX*.5f-90,Canvas->ClipY*.65f,GEngine->GetMediumFont(),1);
        }
    }
    if(Character->IsInventoryOpen())
    {
        const float Left=FMath::Max(12.f,Canvas->ClipX-380.f), Top=205.f;
        DrawRect(FLinearColor(0,0,0,.85f),Left,Top,365,420);
        DrawText(Snapshot.bFinalized?TEXT("ITENS MANTIDOS NESTA PARTIDA"):TEXT("INVENTARIO"),FLinearColor::White,Left+12,Top+10,GEngine->GetMediumFont(),1);
        for(int32 Slot=0;Slot<Marau6Slots::Total;++Slot)
        {
            FString Label=Slot==Marau6Slots::Weapon?TEXT("ARMA") : Marau6Slots::IsSafe(Slot)
                ? FString::Printf(TEXT("SAFE %d"),Slot-Marau6Slots::SafeStart+1):FString::Printf(TEXT("MOCHILA %d"),Slot+1);
            const auto* Item=Inventory->ItemAt(Slot);
            FString Text=Label+TEXT(": ")+(Item?Item->Name():TEXT("--"));
            if(Item && Item->IsRifle()) Text+=FString::Printf(TEXT(" [%d / %d]"),Item->Magazine,Item->Reserve);
            const bool bSelected=Slot==Character->GetSelectedInventorySlot();
            if(bSelected) DrawRect(FLinearColor(.2f,.2f,0,.9f),Left+6,Top+45+Slot*24,353,23);
            DrawText(Text,bSelected?FLinearColor::Yellow:Marau6Slots::IsSafe(Slot)?FLinearColor::Green:FLinearColor::White,
                Left+12,Top+48+Slot*24,GEngine->GetSmallFont(),1);
        }
        DrawText(TEXT("Setas: selecionar | F: equipar"),FLinearColor::White,Left+12,Top+325,GEngine->GetSmallFont(),1);
        DrawText(TEXT("G: descartar | P: safe / mochila (sem rifles)"),FLinearColor::White,Left+12,Top+349,GEngine->GetSmallFont(),1);
        DrawText(Snapshot.bFinalized?TEXT("Resultado guardado ate encerrar o Play"):TEXT("Mochila: 8 | Safe: 2 | Arma: 1"),FLinearColor::White,Left+12,Top+381,GEngine->GetSmallFont(),1);
    }

}

