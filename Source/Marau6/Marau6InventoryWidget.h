#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Marau6InventoryWidget.generated.h"

UCLASS()
class UMarau6InventoryWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry&,const FKeyEvent&) override;
    float GroupWidth(int32 Group) const;
    FString DetailText() const;
};
