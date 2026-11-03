#include "Marau6LootActor.h"
#include "Marau6ItemLedger.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AMarau6LootActor::AMarau6LootActor()
{
    bReplicates=true;PrimaryActorTick.bCanEverTick=false;
    Bounds=CreateDefaultSubobject<UBoxComponent>(TEXT("LootBounds"));SetRootComponent(Bounds);
    Bounds->SetBoxExtent(FVector(30,30,20));Bounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Bounds->SetCollisionResponseToAllChannels(ECR_Ignore);Bounds->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Bounds->SetGenerateOverlapEvents(false);
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LootMesh"));Mesh->SetupAttachment(Bounds);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCanEverAffectNavigation(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Rifle(TEXT("/Game/Weapons/Lasgun/Meshes/SM_Lasgun.SM_Lasgun"));
    BoxMesh=Cube.Object;RifleMesh=Rifle.Object;Mesh->SetStaticMesh(BoxMesh);Mesh->SetRelativeScale3D(FVector(.35,.35,.25));
    Label=CreateDefaultSubobject<UTextRenderComponent>(TEXT("LootLabel"));Label->SetupAttachment(Bounds);
    Label->SetRelativeLocation(FVector(0,0,40));Label->SetRelativeRotation(FRotator(0,180,0));
    Label->SetHorizontalAlignment(EHTA_Center);Label->SetWorldSize(15);Label->SetTextRenderColor(FColor::Yellow);Label->SetText(FText::FromString(TEXT("LOOT")));
}
void AMarau6LootActor::BeginPlay()
{
    Super::BeginPlay();
    if(HasAuthority() && !Item.Id.IsValid()) if(auto* L=GetWorld()->GetSubsystem<UMarau6ItemLedger>()) L->RegisterSeed(this,SeedKind);
    OnRep_Item();
}
void AMarau6LootActor::SetItem(const FMarau6Item& Value) { if(HasAuthority()){Item=Value;OnRep_Item();ForceNetUpdate();} }
void AMarau6LootActor::OnRep_Item()
{
    if(!Item.Id.IsValid()) return;
    Mesh->SetStaticMesh(Item.IsRifle()?RifleMesh:BoxMesh);
    Mesh->SetRelativeScale3D(Item.IsRifle()?FVector(.8):FVector(.35,.35,.25));
    Mesh->SetRelativeRotation(Item.IsRifle()?FRotator(0,-90,0):FRotator::ZeroRotator);
    Bounds->SetBoxExtent(Item.IsRifle()?FVector(60,30,20):FVector(30,30,20));
    Label->SetText(FText::FromString(Item.IsAmmo()?TEXT("Municao | 20 balas"):Item.Name()));
    if(Item.IsAmmo())Mesh->SetRelativeScale3D(FVector(.26,.20,.18));
}
void AMarau6LootActor::Consume() { if(HasAuthority()){SetActorHiddenInGame(true);SetActorEnableCollision(false);Destroy();} }
void AMarau6LootActor::EndPlay(EEndPlayReason::Type Reason)
{
    if(HasAuthority() && GetWorld() && !GetWorld()->bIsTearingDown)
        if(auto* L=GetWorld()->GetSubsystem<UMarau6ItemLedger>()) L->LootRemoved(this,Item.Id);
    Super::EndPlay(Reason);
}
void AMarau6LootActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AMarau6LootActor,Item); }
