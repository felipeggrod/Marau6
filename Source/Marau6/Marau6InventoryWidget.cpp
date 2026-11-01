#include "Marau6InventoryWidget.h"
#include "Marau6PlayerController.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Marau6InventorySlot.h"
#include "Marau6InventoryUIModel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"

float UMarau6InventoryWidget::GroupWidth(int32 Group) const
{
    const auto* PC=GetOwningPlayer<AMarau6PlayerController>();
    float W=GetCachedGeometry().GetLocalSize().X*UWidgetLayoutLibrary::GetViewportScale(this);if(W<100){int32 X=1024,Y=720;if(PC)PC->GetViewportSize(X,Y);W=FMath::Max(640,X);}
    W=FMath::Max(440.f,W-70.f);
    if(PC && !PC->IsInventoryAtBase())return W*(Group==0?.52f:.43f);
    if(W<850)return Group==2?W*.98f:W*.47f;
    return W*(Group==0?.30f:Group==1?.35f:.28f);
}
FString UMarau6InventoryWidget::DetailText() const
{
    const auto* PC=GetOwningPlayer<AMarau6PlayerController>();if(!PC)return FString();
    if(const auto* I=PC->InventoryUIItemAt(PC->GetInventoryUISelection()))return FMarau6InventoryUIModel::Details(*I);
    return TEXT("SLOT VAZIO\n\nSelecione um item para ver os detalhes.");
}
TSharedRef<SWidget> UMarau6InventoryWidget::RebuildWidget()
{
    SetIsFocusable(true);TWeakObjectPtr<AMarau6PlayerController> PC=GetOwningPlayer<AMarau6PlayerController>();
    auto Heading=[](const TCHAR* Label){return SNew(STextBlock).Text(FText::FromString(Label)).Font(FCoreStyle::GetDefaultFontStyle("Bold",17)).ColorAndOpacity(FLinearColor(.87f,.94f,.9f,1));};
    auto Grid=[PC](int32 Start,int32 Count,int32 Columns)
    {
        auto G=SNew(SUniformGridPanel).SlotPadding(FMargin(3));
        for(int32 N=0;N<Count;++N)G->AddSlot(N%Columns,N/Columns)[SNew(SMarau6InventorySlot).Controller(PC).Slot(Start+N)];return G;
    };
    auto KeyButton=[PC](const TCHAR* Label,FKey Key)
    {return SNew(SButton).IsFocusable(false).ContentPadding(FMargin(8,5)).OnClicked_Lambda([PC,Key]{if(auto* P=PC.Get())P->HandleInventoryKey(Key);return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(Label)).Font(FCoreStyle::GetDefaultFontStyle("Regular",14))];};
    auto Root=SNew(SVerticalBox);
    Root->AddSlot().AutoHeight().Padding(8)[SNew(SHorizontalBox)
        +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text_Lambda([PC]{return FText::FromString(PC.IsValid() && PC->IsInventoryAtBase()?FString::Printf(TEXT("MARAU6 | BASE | PERFIL %d"),PC->GetBaseProfile()):TEXT("MARAU6 | INVENTARIO"));}).Font(FCoreStyle::GetDefaultFontStyle("Bold",20)).ColorAndOpacity(FLinearColor::White)]
        +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text_Lambda([PC]{return FText::FromString(PC.IsValid() && PC->IsInventoryReadOnly()?TEXT("SOMENTE LEITURA"):TEXT("ARRASTAR E SOLTAR"));}).Font(FCoreStyle::GetDefaultFontStyle("Regular",12)).ColorAndOpacity(FLinearColor(.6f,.8f,.68f,1))]];
    Root->AddSlot().AutoHeight().Padding(8)[SNew(SHorizontalBox).Visibility_Lambda([PC]{return PC.IsValid() && PC->IsInventoryAtBase()?EVisibility::Visible:EVisibility::Collapsed;})
        +SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[KeyButton(TEXT("1: Perfil 1"),EKeys::One)]
        +SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[KeyButton(TEXT("2: Perfil 2"),EKeys::Two)]
        +SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[KeyButton(TEXT("B: Entrar"),EKeys::B)]
        +SHorizontalBox::Slot().AutoWidth()[KeyButton(TEXT("N: Nova rodada"),EKeys::N)]];
    auto Groups=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(14,18));
    Groups->AddSlot().VAlign(VAlign_Top)[SNew(SBox).WidthOverride_Lambda([this]{return GroupWidth(0);})[SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)[Heading(TEXT("ARMA EQUIPADA"))]
        +SVerticalBox::Slot().AutoHeight()[Grid(10,1,1)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,8)[Heading(TEXT("MOCHILA | 8 SLOTS"))]
        +SVerticalBox::Slot().AutoHeight()[Grid(0,8,4)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,8)[Heading(TEXT("SAFE POCKET | 2 SLOTS"))]
        +SVerticalBox::Slot().AutoHeight()[Grid(8,2,2)]
        +SVerticalBox::Slot().AutoHeight().Padding(4,10)[SNew(STextBlock).Text(FText::FromString(TEXT("Protegidos na morte. Rifles nao cabem no safe."))).AutoWrapText(true).ColorAndOpacity(FLinearColor(.65f,.78f,.7f,1))]]];
    Groups->AddSlot().VAlign(VAlign_Top)[SNew(SBox).WidthOverride_Lambda([this]{return GroupWidth(1);}).Visibility_Lambda([PC]{return PC.IsValid() && PC->IsInventoryAtBase()?EVisibility::Visible:EVisibility::Collapsed;})[SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)[Heading(TEXT("STASH | 32 SLOTS"))]
        +SVerticalBox::Slot().AutoHeight()[Grid(100,32,4)]]];
    Groups->AddSlot().VAlign(VAlign_Top)[SNew(SBox).WidthOverride_Lambda([this]{return GroupWidth(2);})[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.018f,.028f,.022f,1)).Padding(16)
        [SNew(STextBlock).Text_Lambda([this]{return FText::FromString(DetailText());}).Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).AutoWrapText(true).ColorAndOpacity(FLinearColor(.91f,.95f,.92f,1))]]];
    Root->AddSlot().FillHeight(1).Padding(8)[SNew(SScrollBox)+SScrollBox::Slot()[Groups]];
    Root->AddSlot().AutoHeight().Padding(8)[SNew(STextBlock).Text_Lambda([PC]{return FText::FromString(PC.IsValid()?PC->GetInventoryNotice():FString());}).Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).AutoWrapText(true).ColorAndOpacity(FLinearColor(.91f,.85f,.54f,1))];
    Root->AddSlot().AutoHeight().Padding(8,2)[SNew(STextBlock).Text_Lambda([PC]{return FText::FromString(PC.IsValid() && PC->IsInventoryAtBase()?PC->GetBaseBackendStatus():TEXT("O jogo continua: dano, bots e extracao permanecem ativos."));}).Font(FCoreStyle::GetDefaultFontStyle("Regular",12)).AutoWrapText(true).ColorAndOpacity(FLinearColor(.65f,.78f,.7f,1))];
    Root->AddSlot().AutoHeight().Padding(8,5)[SNew(STextBlock).Text_Lambda([PC]{return FText::FromString(PC.IsValid() && PC->IsInventoryAtBase()?TEXT("Clique: detalhes | Arrastar: mover/trocar | Setas + Enter: mover"):TEXT("TAB: fechar | Setas + Enter: mover | F: equipar | G: descartar | P: safe | H: usar"));}).Font(FCoreStyle::GetDefaultFontStyle("Regular",12)).AutoWrapText(true).ColorAndOpacity(FLinearColor(.73f,.83f,.77f,1))];
    // Preserve readable physical text sizes; reflow and scroll instead of shrinking
    // every slot when the project's viewport DPI curve scales down a small window.
    return SNew(SDPIScaler).DPIScale_Lambda([this]{return 1.f/FMath::Max(.25f,UWidgetLayoutLibrary::GetViewportScale(this));})
        [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.008f,.012f,.010f,1)).Padding(12)[Root]];
}
FReply UMarau6InventoryWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E)
{if(E.IsRepeat() && E.GetKey()!=EKeys::Up && E.GetKey()!=EKeys::Down && E.GetKey()!=EKeys::Left && E.GetKey()!=EKeys::Right)return FReply::Handled();
if(auto* PC=GetOwningPlayer<AMarau6PlayerController>();PC && PC->HandleInventoryKey(E.GetKey()))return FReply::Handled();return Super::NativeOnPreviewKeyDown(G,E);}
