#include "Marau6Bots.h"
#include "Marau6Bot.h"
#include "Marau6Persistence.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UMarau6Bots::ResetForRound(const FString& Round)
{
    if(!GetWorld() || GetWorld()->GetNetMode()==NM_Client || GetWorld()->bIsTearingDown || CurrentRound==Round)return;
    TArray<AMarau6Bot*> Old;for(TActorIterator<AMarau6Bot> It(GetWorld());It;++It)Old.Add(*It);
    for(auto* B:Old){if(auto* AI=B->GetController())AI->Destroy();B->Destroy();}
    Spawned.Empty();CurrentRound=Round;NextCheck=0;
}
void UMarau6Bots::Tick(float Delta)
{
    UWorld* W=GetWorld();
    if(!W || W->bIsTearingDown || W->GetNetMode()==NM_Client || !UMarau6Persistence::UsesPersistentBase(W))return;
#if WITH_EDITOR
    // Existing persistence regression fixtures intentionally omit adversaries.
    if(FParse::Param(FCommandLine::Get(),TEXT("Marau6Integration")) && !FParse::Param(FCommandLine::Get(),TEXT("Marau6BotsTest")))return;
#endif
    NextCheck-=Delta;if(NextCheck>0)return;NextCheck=1;
    auto* Bridge=W->GetSubsystem<UMarau6Persistence>();if(!Bridge || !Bridge->CanMutate() || Bridge->GetRoundId().IsEmpty())return;
    ResetForRound(Bridge->GetRoundId());
    auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(W);if(!Nav || !Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))return;
    TArray<ATargetPoint*> Points;for(TActorIterator<ATargetPoint> It(W);It;++It)if(It->ActorHasTag(TEXT("Marau6BotSpawn")))Points.Add(*It);
    Points.Sort([](const ATargetPoint& A,const ATargetPoint& B){return A.GetFName().LexicalLess(B.GetFName());});
    for(auto* Marker:Points)
    {
        if(Spawned.Num()>=3)break;if(Spawned.Contains(Marker->GetFName()))continue;
        for(int32 Attempt=0;Attempt<17;++Attempt)
        {
            const float Angle=(Attempt-1)*PI/4,Radius=Attempt==0?0:Attempt<=8?150:300;
            const FVector Offset(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,300);FNavLocation Ground;
            if(!Nav->ProjectPointToNavigation(Marker->GetActorLocation()+Offset,Ground,FVector(80,80,650)))continue;
            const FVector Position=Ground.Location+FVector(0,0,98);bool NearEntry=false;
            for(TActorIterator<APlayerStart> It(W);It;++It)if(FVector::DistSquared2D(Position,It->GetActorLocation())<FMath::Square(800.f))NearEntry=true;
            if(NearEntry || W->OverlapBlockingTestByProfile(Position,FQuat::Identity,TEXT("Pawn"),FCollisionShape::MakeCapsule(35,97)))continue;
            FActorSpawnParameters S;S.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
            if(auto* B=W->SpawnActor<AMarau6Bot>(AMarau6Bot::StaticClass(),Position,Marker->GetActorRotation(),S))
            {Spawned.Add(Marker->GetFName());UE_LOG(LogTemp,Display,TEXT("MARAU6_BOT spawn=%s id=%s round=%s"),*Position.ToString(),*B->GetLootId().ToString(),*CurrentRound);break;}
        }
    }
}
