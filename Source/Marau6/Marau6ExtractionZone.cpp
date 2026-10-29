#include "Marau6ExtractionZone.h"
#include "Marau6Character.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"

AMarau6ExtractionZone::AMarau6ExtractionZone()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ExtractionBounds"));
    Bounds->SetupAttachment(GetRootComponent());
    Bounds->SetRelativeLocation(FVector(0,0,150));
    Bounds->SetBoxExtent(FVector(180,180,150));
    Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bounds->SetGenerateOverlapEvents(false);
    FloorMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorMarker"));
    FloorMarker->SetupAttachment(GetRootComponent());
    FloorMarker->SetRelativeLocation(FVector(0,0,2));
    FloorMarker->SetRelativeScale3D(FVector(3.6f,3.6f,.025f));
    FloorMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FloorMarker->SetGenerateOverlapEvents(false);
    FloorMarker->SetCanEverAffectNavigation(false);
    FloorMarker->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("/Game/Marau6/Effects/M_ExtractionZone.M_ExtractionZone"));
    FloorMarker->SetStaticMesh(Cube.Object);
    FloorMarker->SetMaterial(0,Material.Object);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ExtractionLabel"));
    Label->SetupAttachment(GetRootComponent());
    Label->SetRelativeLocation(FVector(0,0,235));
    Label->SetText(FText::FromString(TEXT("EXTRACAO\nPERMANECA NA AREA")));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(25.f);
    Label->SetTextRenderColor(FColor::Green);
    Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool AMarau6ExtractionZone::ContainsCharacter(const AMarau6Character* Character) const
{
    if (!IsValid(Character) || IsActorBeingDestroyed()) return false;
    // Require the whole capsule to fit; touching an edge is not enough.
    const FTransform Transform = Bounds->GetComponentTransform();
    const FVector Scale = Transform.GetScale3D().GetAbs();
    if (Scale.GetMin() <= KINDA_SMALL_NUMBER) return false;
    const FVector Local = Transform.InverseTransformPosition(Character->GetActorLocation()).GetAbs();
    const float Radius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float Height = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector Extent = Bounds->GetUnscaledBoxExtent();
    return Local.X + Radius / Scale.X <= Extent.X
        && Local.Y + Radius / Scale.Y <= Extent.Y
        && Local.Z + Height / Scale.Z <= Extent.Z;
}
