#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Marau6PrototypeHUD.generated.h"

/** Minimal read-only display for multiplayer combat testing. */
UCLASS()
class AMarau6PrototypeHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};

