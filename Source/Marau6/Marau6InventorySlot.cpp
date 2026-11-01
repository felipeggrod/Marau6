#include "Marau6InventorySlot.h"
#include "Marau6ItemIcon.h"
#include "Widgets/SBoxPanel.h"
#include "Marau6PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"

TSharedRef<FMarau6InventoryDragOperation> FMarau6InventoryDragOperation::New(AMarau6PlayerController* PC,const FMarau6InventoryUIRequest& R)
{
    auto Op=MakeShared<FMarau6InventoryDragOperation>();Op->Controller=PC;Op->Request=R;
    const auto* Item=PC->InventoryUIItemAt(R.Source);Op->Label=Item?Item->Name():TEXT("Item");Op->IconKind=Item?static_cast<int32>(Item->Kind):-1;Op->Construct();return Op;
}
TSharedPtr<SWidget> FMarau6InventoryDragOperation::GetDefaultDecorator() const
{return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.08f,.09f,.085f,1)).Padding(8)
    [SNew(SVerticalBox)+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(SBox).WidthOverride(90).HeightOverride(45)[SNew(SMarau6ItemIcon).Kind(IconKind)]]
    +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(FLinearColor::White)]];}

void FMarau6InventoryDragOperation::OnDrop(bool Handled,const FPointerEvent& Event)
{if(!Handled)if(auto* P=Controller.Get())P->SetInventoryNotice(TEXT("Movimentacao cancelada."));FGameDragDropOperation::OnDrop(Handled,Event);}
void SMarau6InventorySlot::Construct(const FArguments& Args)
{
    PC=Args._Controller;Slot=Args._Slot;
    SetToolTipText(TAttribute<FText>::CreateLambda([this]{auto* P=PC.Get();const auto* I=P?P->InventoryUIItemAt(Slot):nullptr;return I?FText::FromString(I->Name()):FText::GetEmpty();}));
    ChildSlot[SNew(SBox).MinDesiredHeight(62)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor_Lambda([this]{return FSlateColor(SlotColor());}).Padding(FMargin(4,3))
        [SNew(SVerticalBox)+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [SNew(SBox).WidthOverride(60).HeightOverride(30)[SNew(SMarau6ItemIcon).Kind_Lambda([this]{auto* P=PC.Get();const auto* I=P?P->InventoryUIItemAt(Slot):nullptr;return I?static_cast<int32>(I->Kind):-1;})]]
        +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(ItemLabel());}).Font(FCoreStyle::GetDefaultFontStyle("Regular",12)).Justification(ETextJustify::Center).AutoWrapText(true).ColorAndOpacity(FLinearColor(.92f,.96f,.93f,1))]]]];
}
FString SMarau6InventorySlot::ItemLabel() const
{
    auto* P=PC.Get();if(!P)return TEXT("--");const auto* I=P->InventoryUIItemAt(Slot);
    FString Label=Slot==10?TEXT("ARMA"):Slot>=100?FString::Printf(TEXT("%02d"),Slot-99):Slot>=8?FString::Printf(TEXT("SAFE %d"),Slot-7):FString::Printf(TEXT("%d"),Slot+1);
    if(P->GetInventoryKeyboardSourceSlot()==Slot)Label=TEXT("* ORIGEM\n")+Label;
    if(I){if(I->IsAmmo())Label+=TEXT(" | 20 balas");if(I->IsRifle())Label+=Slot==10?FString::Printf(TEXT("\n%d/%d | %.1f%%"),I->Magazine,I->Reserve,I->Durability/100.f):FString::Printf(TEXT("\n%.1f%%"),I->Durability/100.f);}
    else Label+=TEXT("\n--");return Label;
}
FLinearColor SMarau6InventorySlot::SlotColor() const
{
    auto* P=PC.Get();if(!P)return FLinearColor(.018f,.026f,.022f,1);
    if(FSlateApplication::IsInitialized())if(auto Drag=FSlateApplication::Get().GetDragDroppingContent();Drag.IsValid() && Drag->IsOfType<FMarau6InventoryDragOperation>())
    {auto Op=StaticCastSharedPtr<FMarau6InventoryDragOperation>(Drag);if(Op->Controller==PC){if(Op->Request.Source==Slot)return FLinearColor(.065f,.065f,.065f,1);return FLinearColor(.020f,.030f,.025f,1);}}
    return P->GetInventoryUISelection()==Slot?FLinearColor(.045f,.10f,.06f,1):FLinearColor(.020f,.030f,.025f,1);
}
FReply SMarau6InventorySlot::OnMouseButtonDown(const FGeometry&,const FPointerEvent& E)
{
    if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();
    if(auto* P=PC.Get()){P->SelectInventorySlot(Slot);if(P->InventoryUIItemAt(Slot) && !P->IsInventoryReadOnly() && !P->InventoryRequestPending())return FReply::Handled().DetectDrag(SharedThis(this),EKeys::LeftMouseButton);}
    return FReply::Handled();
}
FReply SMarau6InventorySlot::OnDragDetected(const FGeometry&,const FPointerEvent&)
{
    if(auto* P=PC.Get())
    {P->CancelInventoryDrag();FMarau6InventoryUIRequest R;if(P->CaptureInventoryDrag(Slot,R))return FReply::Handled().BeginDragDrop(FMarau6InventoryDragOperation::New(P,R));}
    return FReply::Handled();
}
FReply SMarau6InventorySlot::OnDragOver(const FGeometry&,const FDragDropEvent& E)
{
    auto Op=E.GetOperationAs<FMarau6InventoryDragOperation>();auto* P=PC.Get();if(!Op || Op->Controller!=PC || !P)return FReply::Unhandled();
    const auto Reason=FMarau6InventoryUIModel::MoveReason(P->GetInventoryUIItems(),Op->Request.Source,Slot,P->IsInventoryAtBase());
    P->SetInventoryNotice(Reason.IsEmpty()?TEXT("Solte para mover, trocar ou abastecer o rifle com 20 balas."):Reason);return FReply::Handled();
}
FReply SMarau6InventorySlot::OnDrop(const FGeometry&,const FDragDropEvent& E)
{
    auto Op=E.GetOperationAs<FMarau6InventoryDragOperation>();auto* P=PC.Get();if(!Op || Op->Controller!=PC || !P)return FReply::Unhandled();
    P->SubmitInventoryDrop(Op->Request,Slot);return FReply::Handled();
}

bool SMarau6InventorySlot::IsBlockedDestination() const
{
    auto* P=PC.Get();if(!P || !FSlateApplication::IsInitialized())return false;
    auto Drag=FSlateApplication::Get().GetDragDroppingContent();
    if(!Drag.IsValid() || !Drag->IsOfType<FMarau6InventoryDragOperation>())return false;
    const auto Op=StaticCastSharedPtr<FMarau6InventoryDragOperation>(Drag);
    return Op->Controller==PC && Op->Request.Source!=Slot &&
        !FMarau6InventoryUIModel::MoveReason(P->GetInventoryUIItems(),Op->Request.Source,Slot,P->IsInventoryAtBase()).IsEmpty();
}
int32 SMarau6InventorySlot::OnPaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Culling,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool bEnabled) const
{
    const int32 Painted=SCompoundWidget::OnPaint(Args,Geometry,Culling,Out,Layer,Style,bEnabled);
    if(!IsBlockedDestination())return Painted;
    const FVector2D Size=Geometry.GetLocalSize();const float Inset=5.f;
    TArray<FVector2D> Points;Points.Add(FVector2D(Inset,Inset));Points.Add(Size-FVector2D(Inset,Inset));
    const FLinearColor Cross(.55f,.55f,.55f,.9f);
    FSlateDrawElement::MakeLines(Out,Painted+1,Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,Cross,true,2.f);
    Points[0]=FVector2D(Inset,Size.Y-Inset);Points[1]=FVector2D(Size.X-Inset,Inset);
    FSlateDrawElement::MakeLines(Out,Painted+1,Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,Cross,true,2.f);
    return Painted+1;
}