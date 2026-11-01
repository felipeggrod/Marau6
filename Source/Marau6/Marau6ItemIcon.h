#pragma once
#include "CoreMinimal.h"
#include "Marau6InventoryTypes.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

/** Small code-native silhouettes; same drawing in the slot and cursor decorator. */
class SMarau6ItemIcon : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SMarau6ItemIcon){} SLATE_ATTRIBUTE(int32,Kind) SLATE_END_ARGS()
    void Construct(const FArguments& Args){Kind=Args._Kind;SetVisibility(EVisibility::HitTestInvisible);}
    virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(66,34);}
private:
    TAttribute<int32> Kind;
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle&,bool) const override
    {
        const int32 K=Kind.Get(-1);if(K<0)return L;
        const FVector2D Scale=G.GetLocalSize()/FVector2D(100,50);
        const FLinearColor Metal(.62f,.70f,.68f,1),Dark(.28f,.36f,.33f,1),Accent(.76f,.64f,.38f,1);
        auto Box=[&](float X,float Y,float W,float H,FLinearColor Color){FSlateDrawElement::MakeBox(Out,L+1,G.ToPaintGeometry(FVector2D(W,H)*Scale,FSlateLayoutTransform(FVector2D(X,Y)*Scale)),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,Color);};
        auto Line=[&](TArray<FVector2D> P,FLinearColor Color,float Width){for(auto& V:P)V*=Scale;FSlateDrawElement::MakeLines(Out,L+2,G.ToPaintGeometry(),P,ESlateDrawEffect::None,Color,true,Width);};
        if(K==0)
        {
            Box(5,20,20,14,Dark);Box(23,18,39,13,Metal);Box(60,21,29,5,Dark);Box(87,19,9,9,Metal);
            Box(39,29,12,17,Dark);Box(26,29,7,11,Dark);Box(37,12,10,6,Metal);Box(58,12,4,7,Metal);
            Line({{8,20},{3,35},{23,31}},Metal,2);
        }
        else if(K==1)
        {
            Box(22,14,56,31,Dark);Line({{39,14},{39,8},{61,8},{61,14}},Metal,2);
            Box(44,19,12,21,FLinearColor(.87f,.89f,.81f,1));Box(36,24,28,10,FLinearColor(.87f,.89f,.81f,1));
        }
        else if(K==3)
        {
            for(int N=0;N<3;N++){const float X=23+N*20;Box(X,20,12,24,Accent);Box(X,35,12,3,Dark);Line({{X,20},{X+6,7},{X+12,20}},Metal,3);}
        }
        else
        {
            Line({{27,8},{22,18},{35,28},{47,22},{60,34},{67,42},{77,35},{67,28},{53,15},{55,6}},Metal,6);
            Box(27,34,12,10,Dark);Line({{32,34},{48,18}},Accent,5);
        }
        return L+2;
    }
};
