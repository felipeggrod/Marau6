#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Input/DragAndDrop.h"
#include "Marau6InventoryUIModel.h"
class AMarau6PlayerController;

class FMarau6InventoryDragOperation : public FGameDragDropOperation
{
public:
    DRAG_DROP_OPERATOR_TYPE(FMarau6InventoryDragOperation,FGameDragDropOperation)
    TWeakObjectPtr<AMarau6PlayerController> Controller;
    FMarau6InventoryUIRequest Request;
    FString Label;
    static TSharedRef<FMarau6InventoryDragOperation> New(AMarau6PlayerController* PC,const FMarau6InventoryUIRequest& R);
    virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;
    virtual void OnDrop(bool Handled,const FPointerEvent& Event) override;
};

class SMarau6InventorySlot : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMarau6InventorySlot){}
        SLATE_ARGUMENT(TWeakObjectPtr<AMarau6PlayerController>,Controller)
        SLATE_ARGUMENT(int32,Slot)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    int32 GetSlotIndex() const {return Slot;}
    virtual FReply OnMouseButtonDown(const FGeometry&,const FPointerEvent&) override;
    virtual FReply OnDragDetected(const FGeometry&,const FPointerEvent&) override;
    virtual FReply OnDragOver(const FGeometry&,const FDragDropEvent&) override;
    virtual FReply OnDrop(const FGeometry&,const FDragDropEvent&) override;
private:
    TWeakObjectPtr<AMarau6PlayerController> PC;
    int32 Slot=INDEX_NONE;
    FString ItemLabel() const;
    FLinearColor SlotColor() const;
    bool IsBlockedDestination() const;
    virtual int32 OnPaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Culling,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool bEnabled) const override;
};
