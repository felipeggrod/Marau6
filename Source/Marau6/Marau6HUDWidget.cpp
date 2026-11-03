#include "Marau6HUDWidget.h"
#include "Marau6PlayerController.h"
#include "Marau6Character.h"
#include "Marau6HealingComponent.h"
#include "Marau6InventoryComponent.h"
#include "Marau6InventoryUIModel.h"
#include "Marau6LootActor.h"
#include "Marau6ItemIcon.h"
#include "Marau6UIStyle.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
/** Segmented gauge: dim empty segments, the filled share in the given color. */
class SMarau6Meter : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SMarau6Meter){}
        SLATE_ATTRIBUTE(float,Fill)
        SLATE_ATTRIBUTE(FLinearColor,Color)
        SLATE_ARGUMENT_DEFAULT(int32,Segments){20};
    SLATE_END_ARGS()
    void Construct(const FArguments& A){Fill=A._Fill;Color=A._Color;Segments=FMath::Max(1,A._Segments);SetVisibility(EVisibility::HitTestInvisible);}
    virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(260,8);}
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle&,bool) const override
    {
        const FVector2D Size=G.GetLocalSize();const float Gap=3.f,Seg=(Size.X-Gap*(Segments-1))/Segments;
        const float Filled=FMath::Clamp(Fill.Get(0.f),0.f,1.f)*Segments;const FLinearColor Ink=Color.Get(Marau6UI::Brass());
        for(int32 N=0;N<Segments;N++)
        {
            const FVector2D At(N*(Seg+Gap),0);
            FSlateDrawElement::MakeBox(Out,L+1,G.ToPaintGeometry(FVector2D(Seg,Size.Y),FSlateLayoutTransform(At)),Marau6UI::White(),ESlateDrawEffect::None,Marau6UI::SRGB(70,62,48,.55f));
            const float Part=FMath::Clamp(Filled-N,0.f,1.f);
            if(Part>0)FSlateDrawElement::MakeBox(Out,L+2,G.ToPaintGeometry(FVector2D(Seg*Part,Size.Y),FSlateLayoutTransform(At)),Marau6UI::White(),ESlateDrawEffect::None,Ink);
        }
        return L+2;
    }
private:
    TAttribute<float> Fill;
    TAttribute<FLinearColor> Color;
    int32 Segments=20;
};
}

UMarau6HUDWidget::UMarau6HUDWidget(const FObjectInitializer& Initializer):Super(Initializer)
{
    static ConstructorHelpers::FObjectFinder<UTexture2D> Rifle(TEXT("/Game/Marau6/UI/Icons/T_UI_Icon_Rifle.T_UI_Icon_Rifle"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> Medkit(TEXT("/Game/Marau6/UI/Icons/T_UI_Icon_Medkit.T_UI_Icon_Medkit"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> Scrap(TEXT("/Game/Marau6/UI/Icons/T_UI_Icon_Scrap.T_UI_Icon_Scrap"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> Ammo(TEXT("/Game/Marau6/UI/Icons/T_UI_Icon_Ammo.T_UI_Icon_Ammo"));
    RifleIcon=Rifle.Object;MedkitIcon=Medkit.Object;ScrapIcon=Scrap.Object;AmmoIcon=Ammo.Object;
}
AMarau6Character* UMarau6HUDWidget::Pawn() const
{const auto* PC=GetOwningPlayer();return PC?Cast<AMarau6Character>(PC->GetPawn()):nullptr;}
UTexture2D* UMarau6HUDWidget::IconFor(int32 Index) const
{
    const auto* C=Pawn();const auto* I=C?C->GetInventory()->ItemAt(Index):nullptr;if(!I)return nullptr;
    return I->IsRifle()?RifleIcon:I->IsAmmo()?AmmoIcon:I->Kind==EMarau6ItemKind::Supply?MedkitIcon:ScrapIcon;
}
FString UMarau6HUDWidget::Objective() const
{
    const auto* C=Pawn();if(!C)return FString();
    switch(C->GetRaidState())
    {
        case EMarau6RaidState::Extracting:return FString::Printf(TEXT("Extraindo: %.1f s. Sair da \u00E1rea cancela."),C->GetExtractionSecondsRemaining());
        case EMarau6RaidState::Extracted:return TEXT("Extra\u00EDdo com sucesso.");
        default:return C->IsAlive()?TEXT("Procure a \u00E1rea verde de extra\u00E7\u00E3o e permane\u00E7a nela."):TEXT("Eliminado.");
    }
}
FString UMarau6HUDWidget::Status() const
{
    const auto* C=Pawn();if(!C)return FString();
    if(!C->GetHealing()->Status().IsEmpty())return C->GetHealing()->Status();
    const auto& Snapshot=C->GetInventory()->GetSnapshot();
    return Snapshot.bFinalized?FString::Printf(TEXT("Itens mantidos: %d \u00B7 perdidos: %d"),Snapshot.Items.Num(),Snapshot.LostCount):FString();
}
TSharedRef<SWidget> UMarau6HUDWidget::RebuildWidget()
{
    TWeakObjectPtr<UMarau6HUDWidget> Self(this);
    auto Char=[Self]{return Self.IsValid()?Self->Pawn():nullptr;};
    auto Label=[](const FString& Text,const FSlateFontInfo& Font,const FLinearColor& Color){return SNew(STextBlock).Text(FText::FromString(Text)).Font(Font).ColorAndOpacity(Color);};
    auto Live=[](TFunction<FString()> Get,const FSlateFontInfo& Font,TFunction<FLinearColor()> Color)
    {return SNew(STextBlock).Text_Lambda([Get]{return FText::FromString(Get());}).Font(Font).ColorAndOpacity_Lambda([Color]{return FSlateColor(Color());});};
    auto Fixed=[](FLinearColor C){return [C]{return C;};};
    auto Cap=[Label](const TCHAR* Key,float Width)
    {return SNew(SBox).WidthOverride(Width).HeightOverride(30)[SNew(SBorder).BorderImage(Marau6UI::KeyCap()).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0)[Label(Key,Marau6UI::Font(TEXT("Medium"),10),Marau6UI::Text())]];};

    // Health: number, segmented gauge and the healing progress while a supply is in use.
    auto Health=[Char]{const auto* C=Char();return C?C->GetHealth():0.f;};
    auto HealthInk=[Health]{return Health()<=30.f?Marau6UI::Fault():Marau6UI::Brass();};
    auto Healing=[Char]{const auto* C=Char();return C && C->GetHealing()->IsHealing();};
    auto HealthPanel=SNew(SBox).WidthOverride(300)[SNew(SBorder).BorderImage(Marau6UI::Panel()).Padding(FMargin(16,12,16,14))[SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1)[Label(TEXT("VIDA"),Marau6UI::Mono(7.5f),Marau6UI::BrassText())]
            +SHorizontalBox::Slot().AutoWidth()[Live([Char,Healing]{const auto* C=Char();return Healing()?FString::Printf(TEXT("CURANDO %.1fs \u00B7 H CANCELA"),C->GetHealing()->Remaining()):FString();},Marau6UI::Mono(7),Fixed(Marau6UI::Waiting()))]]
        +SVerticalBox::Slot().AutoHeight().Padding(0,2,0,0)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth()[Live([Health]{return FString::Printf(TEXT("%.0f"),FMath::CeilToFloat(Health()));},Marau6UI::Font(TEXT("BoldCondensed"),30,20),[Health]{return Health()<=30.f?Marau6UI::Fault():Marau6UI::Text();})]
            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(8,0,0,9)[Label(TEXT("/ 100"),Marau6UI::Mono(8),Marau6UI::Muted())]]
        +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,0)[SNew(SBox).HeightOverride(8)[SNew(SMarau6Meter).Fill_Lambda([Health]{return Health()/100.f;}).Color_Lambda(HealthInk)]]
        +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,0)[SNew(SBox).HeightOverride(3).Visibility_Lambda([Healing]{return Healing()?EVisibility::HitTestInvisible:EVisibility::Collapsed;})
            [SNew(SMarau6Meter).Segments(1).Fill_Lambda([Char,Healing]{return Healing()?1.f-Char()->GetHealing()->Remaining()/4.f:0.f;}).Color(Marau6UI::Ready())]]]];

    // Hotbar: the equipped weapon and the eight backpack slots, mirroring the inventory selection.
    auto Item=[Char](int32 Index)->const FMarau6Item*{const auto* C=Char();return C?C->GetInventory()->ItemAt(Index):nullptr;};
    auto Cell=[Self,Char,Item,Live,Fixed](int32 Index,float Width)
    {
        const bool bWeapon=Index==Marau6Slots::Weapon;
        return SNew(SBox).WidthOverride(Width).HeightOverride(68)[SNew(SBorder).Padding(1)
            .BorderImage_Lambda([Char,Index]{const auto* C=Char();return C && C->GetHotbarArmedSlot()==Index?Marau6UI::SlotBoxSelected():Marau6UI::SlotBox();})
            [SNew(SOverlay)
                +SOverlay::Slot().Padding(bWeapon?FMargin(10,8,10,18):FMargin(8,14,8,14))[SNew(SMarau6ItemIcon)
                    .Kind_Lambda([Item,Index]{const auto* I=Item(Index);return I?static_cast<int32>(I->Kind):-1;})
                    .Texture_Lambda([Self,Index]{return Self.IsValid()?Self->IconFor(Index):nullptr;})]
                +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(6,4,0,0)
                    [Live([Index,bWeapon]{return bWeapon?FString(TEXT("ARMA")):FString::Printf(TEXT("%d"),Index+1);},bWeapon?Marau6UI::Mono(6.5f):Marau6UI::Font(TEXT("Medium"),8.5f),Fixed(bWeapon?Marau6UI::Muted():Marau6UI::Text()))]
                +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                    [SNew(SBox).Visibility_Lambda([Item,Index]{return Item(Index)?EVisibility::Collapsed:EVisibility::HitTestInvisible;})[Live([]{return FString(TEXT("\u2014"));},Marau6UI::Font(TEXT("Regular"),11),Fixed(Marau6UI::Muted()))]]
                +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0,0,0,4)
                    [Live([Item,Index,Char]{const auto* I=Item(Index);if(!I)return FString();return I->IsRifle()?FString::Printf(TEXT("%d / %d"),Char()->GetMagazineAmmo(),Char()->GetMagazineCapacity()):FMarau6InventoryUIModel::Category(*I);},Marau6UI::Mono(6),Fixed(Marau6UI::BrassText()))]]];
    };
    auto Slots=SNew(SHorizontalBox)+SHorizontalBox::Slot().AutoWidth().Padding(0,0,10,0)[Cell(Marau6Slots::Weapon,150)];
    for(int32 Index=0;Index<Marau6Slots::BackpackCount;Index++)Slots->AddSlot().AutoWidth().Padding(Index?6:0,0,0,0)[Cell(Index,68)];
    auto HotbarHint=[Char]
    {
        const auto* C=Char();if(!C)return FString();
        const FString Notice=C->GetHotbarNotice();if(!Notice.IsEmpty())return Notice.ToUpper();
        const int32 Armed=C->GetHotbarArmedSlot();const auto* I=Armed>=0?C->GetInventory()->ItemAt(Armed):nullptr;
        const FString Action=I?FMarau6InventoryUIModel::HotbarAction(*I):FString();
        return Action.IsEmpty()?FString():FString::Printf(TEXT("%d DE NOVO \u00B7 %s"),Armed+1,*Action);
    };
    auto Hotbar=SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0,0,0,8)[SNew(SBox).Visibility_Lambda([HotbarHint]{return HotbarHint().IsEmpty()?EVisibility::Hidden:EVisibility::HitTestInvisible;})
            [SNew(SBorder).BorderImage(Marau6UI::Panel()).Padding(FMargin(12,5))[Live(HotbarHint,Marau6UI::Mono(7.5f),Fixed(Marau6UI::Brass()))]]]
        +SVerticalBox::Slot().AutoHeight()[SNew(SBorder).BorderImage(Marau6UI::Panel()).Padding(FMargin(10,10,16,10))[SNew(SHorizontalBox)
        +SHorizontalBox::Slot().AutoWidth()[Slots]
        +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(22,0,0,0)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Cap(TEXT("TAB"),48)]
            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10,0,0,0)[SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight()[Label(TEXT("INVENT\u00C1RIO"),Marau6UI::Font(TEXT("Regular"),10.5f,40),Marau6UI::Text())]
                +SVerticalBox::Slot().AutoHeight().Padding(0,3,0,0)[Label(TEXT("ABRIR / FECHAR"),Marau6UI::Mono(6.5f),Marau6UI::Muted())]]]]];

    // Ammo: magazine against capacity, then everything carried (reserve plus unopened boxes).
    auto Armed=[Char]{const auto* C=Char();return C && C->IsAlive() && C->HasEquippedRifle();};
    auto Carried=[Char]
    {
        const auto* C=Char();if(!C)return 0;int32 Boxes=0;
        for(int32 Index=0;Index<Marau6Slots::BackpackCount;++Index)if(const auto* I=C->GetInventory()->ItemAt(Index))Boxes+=I->IsAmmo()?1:0;
        return C->GetReserveAmmo()+Boxes*Marau6Ammo::BoxRounds;
    };
    auto WeaponState=[Char,Armed]
    {
        const auto* C=Char();if(!C || !C->IsAlive())return FString();
        if(!C->HasEquippedRifle())return FString(TEXT("SEM ARMA \u00B7 EQUIPE PELO INVENT\u00C1RIO"));
        if(C->GetRifleDurability()<=0)return FString(TEXT("ARMA N\u00C3O DISPARA"));
        if(C->GetHealing()->IsHealing())return FString(TEXT("TIRO BLOQUEADO DURANTE A CURA"));
        if(C->IsReloading())return FString(TEXT("RECARREGANDO"));
        return C->GetMagazineAmmo()==0 && C->GetReserveAmmo()==0?FString(TEXT("SEM MUNI\u00C7\u00C3O")):FString();
    };
    auto AmmoPanel=SNew(SBox).WidthOverride(240)[SNew(SBorder).BorderImage(Marau6UI::Panel()).Padding(FMargin(16,12,16,14))[SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1)[Label(TEXT("MUNI\u00C7\u00C3O"),Marau6UI::Mono(7.5f),Marau6UI::BrassText())]
            +SHorizontalBox::Slot().AutoWidth()[Live([Char,Armed]{return Armed()?FString::Printf(TEXT("COND %.1f%%"),Char()->GetRifleDurability()/100.f):FString();},Marau6UI::Mono(7),Fixed(Marau6UI::Muted()))]]
        +SVerticalBox::Slot().AutoHeight().Padding(0,2,0,0)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth()[Live([Char,Armed]{return Armed()?FString::Printf(TEXT("%d"),Char()->GetMagazineAmmo()):FString(TEXT("\u2014"));},Marau6UI::Font(TEXT("BoldCondensed"),30,20),
                [Char,Armed]{return Armed() && Char()->GetMagazineAmmo()==0?Marau6UI::Fault():Marau6UI::Text();})]
            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(8,0,0,9)[Live([Char,Armed]{return Armed()?FString::Printf(TEXT("/ %d"),Char()->GetMagazineCapacity()):FString();},Marau6UI::Mono(8),Fixed(Marau6UI::Muted()))]
            +SHorizontalBox::Slot().FillWidth(1)
            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(0,0,0,9)[Live([Armed,Carried]{return Armed()?FString::Printf(TEXT("RESERVA %d"),Carried()):FString();},Marau6UI::Mono(7.5f),
                [Carried]{return Carried()==0?Marau6UI::Fault():Marau6UI::Text();})]]
        +SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)[SNew(SBox).Visibility_Lambda([WeaponState]{return WeaponState().IsEmpty()?EVisibility::Collapsed:EVisibility::HitTestInvisible;})
            [Live(WeaponState,Marau6UI::Mono(7),Fixed(Marau6UI::Waiting()))]]]];

    // Objective and the latest feedback, top left.
    auto ObjectivePanel=SNew(SBox).MaxDesiredWidth(420)[SNew(SBorder).BorderImage(Marau6UI::Panel()).Padding(FMargin(16,12))[SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[Label(TEXT("OBJETIVO"),Marau6UI::Mono(7),Marau6UI::BrassText())]
        +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,0)[Live([Self]{return Self.IsValid()?Self->Objective():FString();},Marau6UI::Font(TEXT("Medium"),11,20),Fixed(Marau6UI::Text()))]
        +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,0)[SNew(SBox).Visibility_Lambda([Self]{return Self.IsValid() && !Self->Status().IsEmpty()?EVisibility::HitTestInvisible:EVisibility::Collapsed;})
            [SNew(STextBlock).Text_Lambda([Self]{return FText::FromString(Self.IsValid()?Self->Status():FString());}).Font(Marau6UI::Font(TEXT("Regular"),9.5f)).ColorAndOpacity(Marau6UI::Muted()).AutoWrapText(true)]]]];

    // Loot under the crosshair.
    auto LootPrompt=[Char]()->FString
    {
        const auto* C=Char();if(!C || !C->CanParticipate())return FString();
        const auto* Loot=C->GetInventory()->LookedAtLoot();if(!Loot)return FString();
        bool bSpace=Loot->GetItem().IsRifle() && !C->HasEquippedRifle();
        for(int32 Index=0;Index<Marau6Slots::BackpackCount;++Index)if(!C->GetInventory()->ItemAt(Index))bSpace=true;
        return bSpace?TEXT("COLETAR ")+FMarau6InventoryUIModel::ShortLabel(Loot->GetItem())+TEXT(" \u00B7 ")+Loot->GetItem().Name().ToUpper():FString(TEXT("MOCHILA CHEIA"));
    };
    auto Prompt=SNew(SBox).Visibility_Lambda([LootPrompt]{return LootPrompt().IsEmpty()?EVisibility::Collapsed:EVisibility::HitTestInvisible;})
        [SNew(SBorder).BorderImage(Marau6UI::Panel()).Padding(FMargin(10,8,14,8))[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Cap(TEXT("E"),30)]
            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10,0,0,0)[Live(LootPrompt,Marau6UI::Font(TEXT("Regular"),10.5f,40),Fixed(Marau6UI::Text()))]]];

    // Result once the player can no longer act in this raid.
    auto Over=[Char]{const auto* C=Char();return C && !C->CanParticipate();};
    auto Extracted=[Char]{const auto* C=Char();return C && C->GetRaidState()==EMarau6RaidState::Extracted;};
    auto Result=SNew(SBox).Visibility_Lambda([Over]{return Over()?EVisibility::HitTestInvisible:EVisibility::Collapsed;})
        [SNew(SBorder).BorderImage(Marau6UI::Panel()).Padding(FMargin(28,18)).HAlign(HAlign_Center)[SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Live([Extracted]{return Extracted()?FString(TEXT("EXTRA\u00CDDO")):FString(TEXT("ELIMINADO"));},Marau6UI::Font(TEXT("BoldCondensed"),30,40),
                [Extracted]{return Extracted()?Marau6UI::Ready():Marau6UI::Fault();})]
            +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0,6,0,0)[Live([Self]
                {const auto* PC=Self.IsValid()?Cast<AMarau6PlayerController>(Self->GetOwningPlayer()):nullptr;
                return FString(PC && PC->IsUsingBase()?TEXT("SALVANDO RESULTADO \u00B7 RETORNO \u00C0 BASE AP\u00D3S CONFIRMA\u00C7\u00C3O"):TEXT("REINICIE O PLAY PARA UMA NOVA PARTIDA DE TESTE"));},Marau6UI::Mono(7.5f),Fixed(Marau6UI::Muted()))]]];

    auto Screen=SNew(SOverlay)
        +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(28,24)[ObjectivePanel]
        +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0,180,0,0)[Prompt]
        +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0,0,0,220)[Result]
        +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(28,0,0,28)[HealthPanel]
        +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0,0,0,28)[Hotbar]
        +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0,0,28,28)[AmmoPanel];
    // Laid out at 1920x1080 and scaled to the viewport, so the corners never meet the hotbar.
    return SNew(SDPIScaler).Visibility(EVisibility::HitTestInvisible).DPIScale_Lambda([Self]
    {
        if(!Self.IsValid())return 1.f;int32 X=1920,Y=1080;if(const auto* PC=Self->GetOwningPlayer())PC->GetViewportSize(X,Y);
        const float Fit=FMath::Max(.4f,FMath::Min(X/1920.f,Y/1080.f));
        return Fit/FMath::Max(.25f,UWidgetLayoutLibrary::GetViewportScale(Self.Get()));
    })[Screen];
}
