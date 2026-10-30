#include "Marau6PrototypeHUD.h"
#include "Marau6PlayerController.h"
#include "Serialization/JsonSerializer.h"
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
    const auto* BasePC=Cast<AMarau6PlayerController>(PlayerOwner);
    if(BasePC && BasePC->IsUsingBase() && !BasePC->IsInRaid())
    {
        DrawRect(FLinearColor(.025f,.035f,.045f,.98f),0,0,Canvas->ClipX,Canvas->ClipY);
        const float Scale=FMath::Min(Canvas->ClipX/1200.f,Canvas->ClipY/720.f);
        auto Text=[&](const FString& T,float X,float Y,FLinearColor Color=FLinearColor::White)
        {DrawText(T,Color,X*Scale,Y*Scale,GEngine->GetMediumFont(),Scale);};
        Text(FString::Printf(TEXT("MARAU6 | BASE | PERFIL: %d"),BasePC->GetBaseProfile()),25,20);
        Text(TEXT("1: Teste 1 | 2: Teste 2 | B: entrar na partida | N: nova rodada apos ambos terminarem"),25,55);
        Text(TEXT("Cima/baixo: slot | Esquerda/direita: coluna | Enter: marcar origem, depois selecionar destino e Enter novamente"),25,83);
        Text(BasePC->GetBaseMessage(),25,112,FLinearColor::Yellow);
        Text(BasePC->GetBaseBackendStatus(),25,140,FLinearColor(.65f,.75f,.85f));
        TSharedPtr<FJsonObject> State;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(BasePC->GetBaseJson()),State);
        TMap<int32,TSharedPtr<FJsonObject>> Items;
        const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
        if(State && State->TryGetArrayField(TEXT("items"),Values))for(const auto& V:*Values)Items.Add(V->AsObject()->GetIntegerField(TEXT("slot")),V->AsObject());
        Text(TEXT("EQUIPAMENTO PARA A PROXIMA PARTIDA"),25,185);
        Text(TEXT("STASH | 32 SLOTS"),440,185);
        for(int32 Index=0;Index<43;++Index)
        {
            const int32 Slot=AMarau6PlayerController::BaseSlot(Index);
            const float X=Index<11?25:440+((Index-11)/16)*365;
            const float Y=220+(Index<11?Index:(Index-11)%16)*26;
            FString Label=Slot==10?TEXT("ARMA"):Slot==8?TEXT("SAFE 1"):Slot==9?TEXT("SAFE 2"):Slot>=100?FString::Printf(TEXT("STASH %02d"),Slot-99):FString::Printf(TEXT("MOCHILA %d"),Slot+1);
            if(const auto* I=Items.Find(Slot))
            {
                const int32 Kind=(*I)->GetIntegerField(TEXT("kind"));
                Label+=Kind==0?TEXT(": Rifle"):Kind==1?TEXT(": Suprimentos"):TEXT(": Pecas");
                if(Kind==0)Label+=FString::Printf(TEXT(" [%d/%d]"),(*I)->GetIntegerField(TEXT("magazine")),(*I)->GetIntegerField(TEXT("reserve")));
            }
            else Label+=TEXT(": --");
            if(Index==BasePC->GetBaseSelection())DrawRect(FLinearColor(.18f,.2f,.05f,1.f),X*Scale-4,Y*Scale-2,350*Scale,25*Scale);
            if(Index==BasePC->GetBaseSource())Label=TEXT("* ")+Label;
            Text(Label,X,Y,Index==BasePC->GetBaseSelection()?FLinearColor::Yellow:Slot==8 || Slot==9?FLinearColor::Green:FLinearColor::White);
        }
        Text(TEXT("Perfis locais de desenvolvimento. Os dados sao salvos pelo servidor."),25,675,FLinearColor(.6f,.7f,.8f));
        return;
    }

    const AMarau6Character* Character = Cast<AMarau6Character>(PlayerOwner->GetPawn());
    if (!Character) return;

    DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.65f), 12.f, 12.f, 400.f, 210.f);
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
        DrawText(BasePC && BasePC->IsUsingBase()?TEXT("Salvando resultado; retorno a base apos confirmacao"):TEXT("Reinicie o Play para uma nova partida de teste"),FLinearColor::White,
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
            if(Item && Item->Id==Character->GetInventoryMoveSource()) Text=TEXT("* ")+Text;
            const bool bSelected=Slot==Character->GetSelectedInventorySlot();
            if(bSelected) DrawRect(FLinearColor(.2f,.2f,0,.9f),Left+6,Top+45+Slot*24,353,23);
            DrawText(Text,bSelected?FLinearColor::Yellow:Marau6Slots::IsSafe(Slot)?FLinearColor::Green:FLinearColor::White,
                Left+12,Top+48+Slot*24,GEngine->GetSmallFont(),1);
        }
        DrawText(TEXT("Setas: slot | Enter: origem / destino"),FLinearColor::White,Left+12,Top+325,GEngine->GetSmallFont(),1);
        DrawText(TEXT("F: equipar | G: descartar | P: safe / mochila"),FLinearColor::White,Left+12,Top+349,GEngine->GetSmallFont(),1);
        DrawText(Snapshot.bFinalized?TEXT("Resultado guardado ate encerrar o Play"):TEXT("* Origem | Mesmo slot: cancelar | TAB: fechar"),FLinearColor::White,Left+12,Top+381,GEngine->GetSmallFont(),1);
    }

}

