#include "Marau6InventoryUIModel.h"

bool FMarau6InventoryUIModel::ValidSlot(int32 S,bool bBase)
{return (S>=0 && S<Marau6Slots::Total) || (bBase && S>=100 && S<132);}
FString FMarau6InventoryUIModel::SlotName(int32 S)
{
    if(S==10)return TEXT("ARMA EQUIPADA");
    if(S>=100 && S<132)return FString::Printf(TEXT("STASH %02d"),S-99);
    if(S==8 || S==9)return FString::Printf(TEXT("SAFE %d"),S-7);
    return FString::Printf(TEXT("MOCHILA %d"),S+1);
}
FString FMarau6InventoryUIModel::MoveReason(const TArray<FMarau6Item>& Items,int32 S,int32 D,bool bBase)
{
    if(!ValidSlot(S,bBase) || !ValidSlot(D,bBase))return TEXT("Destino indisponivel neste inventario.");
    const auto* A=Items.FindByPredicate([S](const auto& I){return I.Slot==S;});
    const auto* B=Items.FindByPredicate([D](const auto& I){return I.Slot==D;});
    if(!A)return TEXT("O item mudou de posicao; tente novamente.");
    if(S==D)return TEXT("Movimentacao cancelada.");
    auto Accepts=[](int32 Slot,const FMarau6Item& I){return Slot>=100 || Marau6Slots::Accepts(Slot,I);};
    if(!Accepts(D,*A))return Marau6Slots::IsSafe(D)?TEXT("Rifles nao cabem no safe pocket."):TEXT("O espaco de arma aceita somente rifles.");
    if(B && !Accepts(S,*B))return TEXT("Troca incompativel: os dois itens precisam caber nos destinos.");
    return FString();
}
FString FMarau6InventoryUIModel::Details(const FMarau6Item& I)
{
    FString Info=I.Name()+TEXT("\n")+SlotName(I.Slot)+TEXT("\n\n");
    if(I.IsRifle())return Info+FString::Printf(TEXT("Carregador: %d / 10\nReserva: %d\nDurabilidade: %.1f%%\n\n"),I.Magazine,I.Reserve,I.Durability/100.f)
        +(I.Durability<=0?TEXT("Sem condicao de disparar.\n\n"):TEXT("Perde 2,5 pontos por disparo aceito.\n\n"))+TEXT("Recarregar nao repara.\nRifles nao cabem no safe pocket.");
    if(I.Kind==EMarau6ItemKind::Supply)return Info+TEXT("Recupera ate 40 de vida\nTempo: 4 segundos\nUso unico\n\nUse na partida, parado no chao.\nMovimento ou dano interrompem a cura.\nVida cheia nao consome o item.");
    return Info+TEXT("Material coletavel\n\nPode ser guardado e extraido.\nSem uso de crafting neste prototipo.");
}
