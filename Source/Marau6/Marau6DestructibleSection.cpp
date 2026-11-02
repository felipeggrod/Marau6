#include "Marau6DestructibleSection.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NavModifierComponent.h"
#include "NavAreas/NavArea_Null.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AMarau6DestructibleSection::AMarau6DestructibleSection()
{
    bReplicates=true;bAlwaysRelevant=true;SetReplicateMovement(true);PrimaryActorTick.bCanEverTick=false;
    Bounds=CreateDefaultSubobject<UBoxComponent>(TEXT("SectionCollision"));SetRootComponent(Bounds);
    Bounds->SetBoxExtent(FVector(10,60,120));Bounds->SetCollisionProfileName(TEXT("BlockAll"));Bounds->SetGenerateOverlapEvents(false);Bounds->SetCanEverAffectNavigation(true);
    NavigationBlock=CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavigationBlock"));NavigationBlock->SetAreaClass(UNavArea_Null::StaticClass());NavigationBlock->bIncludeAgentHeight=true;NavigationBlock->FailsafeExtent=FVector(10,60,120);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("/Game/Marau6/Effects/M_ShotGlow.M_ShotGlow"));
    auto Mesh=[&](const TCHAR* Name,FVector Position,FVector Scale)
    {
        auto* M=CreateDefaultSubobject<UStaticMeshComponent>(Name);M->SetupAttachment(Bounds);M->SetStaticMesh(Cube.Object);M->SetMaterial(0,Material.Object);M->SetRelativeLocation(Position);M->SetRelativeScale3D(Scale);
        M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetCanEverAffectNavigation(false);return M;
    };
    Body=Mesh(TEXT("BluePanel"),FVector::ZeroVector,FVector(.20,1.20,2.40));
    for(int Side=0;Side<2;Side++)
    {
        const float X=Side==0?-10.8f:10.8f;
        for(int Band=0;Band<2;Band++)Stripes.Add(Mesh(*FString::Printf(TEXT("Stripe%d%d"),Side,Band),FVector(X,0,Band==0?-65:65),FVector(.012,1.16,.07)));
        for(int Mark=0;Mark<2;Mark++)
        {auto* Crack=Mesh(*FString::Printf(TEXT("Crack%d%d"),Side,Mark),FVector(X,Mark==0?-12:15,Mark==0?25:-20),FVector(.014,.035,.80));Crack->SetRelativeRotation(FRotator(0,0,Mark==0?35:-30));Cracks.Add(Crack);}
        auto* Label=CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("SectionLabel%d"),Side));Label->SetupAttachment(Bounds);
        Label->SetRelativeLocation(FVector(X*1.03,0,0));Label->SetRelativeRotation(FRotator(0,Side==0?180:0,0));Label->SetHorizontalAlignment(EHTA_Center);Label->SetVerticalAlignment(EVRTA_TextCenter);
        Label->SetWorldSize(10);Label->SetText(FText::FromString(TEXT("DESTRUTIVEL")));Label->SetTextRenderColor(FColor::White);Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);Label->SetCanEverAffectNavigation(false);Labels.Add(Label);
    }
}
void AMarau6DestructibleSection::BeginPlay()
{
    Super::BeginPlay();BlueMaterial=Body->CreateDynamicMaterialInstance(0);
    for(auto S:Stripes)if(auto* M=S->CreateDynamicMaterialInstance(0))M->SetVectorParameterValue(TEXT("GlowColor"),FLinearColor(.8,.85,.9));
    for(auto C:Cracks)if(auto* M=C->CreateDynamicMaterialInstance(0))M->SetVectorParameterValue(TEXT("GlowColor"),FLinearColor(.006,.009,.014));
    OnRep_Health();
}
void AMarau6DestructibleSection::SetArenaIdentity(int32 Barrier,int32 Section)
{if(HasAuthority()){BarrierIndex=Barrier;SectionIndex=Section;ForceNetUpdate();}}
float AMarau6DestructibleSection::TakeDamage(float Damage,const FDamageEvent& Event,AController* DamageInstigator,AActor* Causer)
{
    if(!HasAuthority() || !CanBeDamaged() || IsBroken() || !FMath::IsFinite(Damage) || Damage<=0 || !GetWorld() || GetWorld()->bIsTearingDown)return 0;
    const float Applied=FMath::Min(Damage,Health);Health=FMath::Max(0.f,Health-Applied);OnRep_Health();ForceNetUpdate();
    if(IsBroken())MulticastBreak(Generation);
    Super::TakeDamage(Applied,Event,DamageInstigator,Causer);return Applied;
}
void AMarau6DestructibleSection::ResetSection()
{if(HasAuthority()){++Generation;Health=200;OnRep_Health();ForceNetUpdate();}}
void AMarau6DestructibleSection::OnRep_Health()
{
    const bool Broken=IsBroken();const bool WasColliding=Bounds->IsCollisionEnabled();Bounds->SetCollisionEnabled(Broken?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);
    Body->SetVisibility(!Broken);for(auto S:Stripes)S->SetVisibility(!Broken);for(auto L:Labels)L->SetVisibility(!Broken);
    for(int32 N=0;N<Cracks.Num();N++)Cracks[N]->SetVisibility(!Broken && (N%2==0?Health<200:Health<=100));
    if(BlueMaterial)BlueMaterial->SetVectorParameterValue(TEXT("GlowColor"),Health<=100?FLinearColor(.025,.12,.50):FLinearColor(.035,.23,.90));
    if(GetNetMode()!=NM_Client && (WasColliding==Broken)){NavigationBlock->SetNavigationRelevancy(!Broken);if(!Broken)NavigationBlock->UpdateNavigationBounds();}
}
void AMarau6DestructibleSection::MulticastBreak_Implementation(uint32 BrokenGeneration)
{
    if(GetNetMode()==NM_DedicatedServer || BrokenGeneration!=Generation)return;
    FActorSpawnParameters S;S.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    GetWorld()->SpawnActor<AMarau6SectionDebris>(GetActorLocation(),GetActorRotation(),S);
}
void AMarau6DestructibleSection::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AMarau6DestructibleSection,Health);DOREPLIFETIME(AMarau6DestructibleSection,Generation);DOREPLIFETIME(AMarau6DestructibleSection,BarrierIndex);DOREPLIFETIME(AMarau6DestructibleSection,SectionIndex);}
AMarau6SectionDebris::AMarau6SectionDebris()
{
    bReplicates=false;PrimaryActorTick.bCanEverTick=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("/Game/Marau6/Effects/M_ShotGlow.M_ShotGlow"));
    for(int32 N=0;N<5;N++)
    {auto* M=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Fragment%d"),N));M->SetupAttachment(GetRootComponent());M->SetStaticMesh(Cube.Object);M->SetMaterial(0,Material.Object);M->SetRelativeScale3D(FVector(.08,.16,.24));M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetCanEverAffectNavigation(false);Pieces.Add(M);}
}
void AMarau6SectionDebris::BeginPlay()
{Super::BeginPlay();SetLifeSpan(.65f);for(auto P:Pieces)if(auto* M=P->CreateDynamicMaterialInstance(0))M->SetVectorParameterValue(TEXT("GlowColor"),FLinearColor(.03,.16,.6));}
void AMarau6SectionDebris::Tick(float Delta)
{
    Super::Tick(Delta);Age+=Delta;
    for(int32 N=0;N<Pieces.Num();N++)
    {Pieces[N]->SetRelativeLocation(FVector((N%2?1:-1)*Age*80,(N-2)*Age*100,(N%3)*25+Age*90-350*Age*Age));Pieces[N]->SetRelativeRotation(FRotator(Age*180,N*38,Age*230));}
}
