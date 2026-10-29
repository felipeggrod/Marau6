#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Marau6ExtractionZone.generated.h"
class AMarau6Character;
class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Static extraction area. Presence is checked by the server, never a client RPC. */
UCLASS()
class AMarau6ExtractionZone : public AActor
{
    GENERATED_BODY()
public:
    AMarau6ExtractionZone();
    bool ContainsCharacter(const AMarau6Character* Character) const;
    float GetHoldSeconds() const { return HoldSeconds; }
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Bounds;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FloorMarker;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(EditAnywhere, Category="Extraction", meta=(ClampMin="1.0", ClampMax="60.0"))
    float HoldSeconds = 5.f;
};
