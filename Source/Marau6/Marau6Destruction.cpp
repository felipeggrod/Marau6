#include "Marau6Destruction.h"
#include "Marau6DestructibleSection.h"
#include "Marau6Persistence.h"
#include "Marau6LootActor.h"
#include "Marau6ExtractionZone.h"
#include "NavigationSystem.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void UMarau6Destruction::OnWorldBeginPlay(UWorld& W)
{Super::OnWorldBeginPlay(W);if(W.GetNetMode()!=NM_Client && UMarau6Persistence::UsesPersistentBase(&W))EnsureArena();}
int32 UMarau6Destruction::GetSectionCount()const
{int32 Count=0;for(const auto& S:Sections)if(S.IsValid())++Count;return Count;}
void UMarau6Destruction::EnsureArena()
{
    UWorld* W=GetWorld();if(!W || W->GetNetMode()==NM_Client || GetSectionCount()>0)return;
    auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(W);if(!Nav || Nav->HasDirtyAreasQueued() || Nav->IsNavigationBuildInProgress())return;
    TArray<FVector> Candidates={FVector(0,-900,300),FVector(900,300,300),FVector(-900,0,300)};
    for(int32 X=-1200;X<=1200;X+=400)for(int32 Y=-1200;Y<=1200;Y+=400)Candidates.Add(FVector(X,Y,300));
    TArray<FVector> Chosen;
    for(const FVector& Candidate:Candidates)
    {
        if(Chosen.Num()==3)break;
        bool Clear=true;for(const auto& C:Chosen)if(FVector::DistSquared2D(Candidate,C)<FMath::Square(650.f))Clear=false;
        for(TActorIterator<APlayerStart> It(W);It;++It)if(FVector::DistSquared2D(Candidate,It->GetActorLocation())<FMath::Square(600.f))Clear=false;
        for(TActorIterator<AMarau6ExtractionZone> It(W);It;++It)if(FVector::DistSquared2D(Candidate,It->GetActorLocation())<FMath::Square(550.f))Clear=false;
        if(!Clear)continue;
        FVector Centers[3];float Ground=0;
        for(int32 N=0;N<3 && Clear;N++)
        {
            FNavLocation P;if(!Nav->ProjectPointToNavigation(Candidate+FVector(0,(N-1)*120,0),P,FVector(45,45,600))){Clear=false;break;}
            FHitResult Floor;FCollisionQueryParams Q(SCENE_QUERY_STAT(Marau6WallGround),false);
            if(!W->LineTraceSingleByChannel(Floor,P.Location+FVector(0,0,50),P.Location-FVector(0,0,100),ECC_Visibility,Q)){Clear=false;break;}
            if(N==0)Ground=Floor.ImpactPoint.Z;if(FMath::Abs(Floor.ImpactPoint.Z-Ground)>5){Clear=false;break;}
            Centers[N]=FVector(Candidate.X,Candidate.Y+(N-1)*120,Floor.ImpactPoint.Z+122);
            if(W->OverlapBlockingTestByChannel(Centers[N],FQuat::Identity,ECC_Pawn,FCollisionShape::MakeBox(FVector(12,61,120)),Q))Clear=false;
            for(TActorIterator<AMarau6LootActor> It(W);It;++It)if(FVector::DistSquared2D(Centers[N],It->GetActorLocation())<FMath::Square(180.f))Clear=false;
            // The path on both sides must already be walkable; no floating scenery.
            for(int Side:{-1,1}){FNavLocation Approach;if(!Nav->ProjectPointToNavigation(Centers[N]+FVector(Side*220,0,-120),Approach,FVector(45,45,80)))Clear=false;}
        }
        if(!Clear)continue;
        const int32 Group=Chosen.Num();Chosen.Add(Candidate);
        for(int32 N=0;N<3;N++)
        {
            const FTransform Transform(FRotator::ZeroRotator,Centers[N]);
            auto* S=W->SpawnActorDeferred<AMarau6DestructibleSection>(AMarau6DestructibleSection::StaticClass(),Transform);
            if(S){S->SetArenaIdentity(Group,N);Sections.Add(S);S->FinishSpawning(Transform);UE_LOG(LogTemp,Display,TEXT("MARAU6_SECTION group=%d section=%d location=%s"),Group,N,*Centers[N].ToString());}
        }
    }
    if(GetSectionCount()!=9)UE_LOG(LogTemp,Error,TEXT("Marau6 destruction arena requires9 collision-free sections, created%d"),GetSectionCount());
}
void UMarau6Destruction::ResetForRound(const FString& Round)
{
    if(!GetWorld() || GetWorld()->GetNetMode()==NM_Client || GetWorld()->bIsTearingDown || Round.IsEmpty() || CurrentRound==Round)return;
    bNavigationReady=false;ResetFrame=GFrameCounter;EnsureArena();for(TActorIterator<AMarau6DestructibleSection> It(GetWorld());It;++It)It->ResetSection();CurrentRound=Round;
    // Flush registration into the octree now; dirty tiles still build asynchronously.
    if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))Nav->ProcessPendingOctreeUpdates();
}

void UMarau6Destruction::Tick(float)
{
    if(!GetWorld() || GetWorld()->GetNetMode()==NM_Client || GetWorld()->bIsTearingDown || CurrentRound.IsEmpty() || bNavigationReady || GFrameCounter<=ResetFrame+2)return;
    auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if(!Nav)return;
    if(GetSectionCount()==0)
    {
        EnsureArena();ResetFrame=GFrameCounter;Nav->ProcessPendingOctreeUpdates();return;
    }
    if(GetSectionCount()!=9)return;
    if(!Nav->HasDirtyAreasQueued() && !Nav->IsNavigationBuildInProgress())bNavigationReady=true;
}
