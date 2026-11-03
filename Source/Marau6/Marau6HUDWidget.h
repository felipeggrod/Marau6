#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Marau6HUDWidget.generated.h"
class UTexture2D;
class AMarau6Character;

/** In-raid HUD in the base screen's style: health, objective, hotbar with the TAB cue, ammo and loot prompt. */
UCLASS()
class UMarau6HUDWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UMarau6HUDWidget(const FObjectInitializer& Initializer);
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    AMarau6Character* Pawn() const;
    UTexture2D* IconFor(int32 Index) const;
    FString Objective() const;
    FString Status() const;
    UPROPERTY() TObjectPtr<UTexture2D> RifleIcon;
    UPROPERTY() TObjectPtr<UTexture2D> MedkitIcon;
    UPROPERTY() TObjectPtr<UTexture2D> ScrapIcon;
    UPROPERTY() TObjectPtr<UTexture2D> AmmoIcon;
};
