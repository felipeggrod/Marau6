#if WITH_EDITOR
#include "Marau6Character.h"
#include "Marau6PlayerController.h"
#include "Engine/World.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
namespace
{
// Run on a map outside the persistent base, where the character starts armed.
// Saves the owner's view at several pitches
// to LocalSettings/viewmodel_*.png. Development inspection only.
void CaptureViewModel(UWorld* World)
{
    if(!World || !World->IsGameWorld()) return;
    TWeakObjectPtr<AMarau6PlayerController> WeakPC(World->GetFirstPlayerController<AMarau6PlayerController>());
    if(!WeakPC.IsValid()) return;
    static const float Pitches[]={0.f,35.f,-35.f,75.f,-65.f};
    struct FState{int32 Phase=0;int32 Shot=0;double At=0;};
    TSharedRef<FState> S=MakeShared<FState>();
    S->At=FPlatformTime::Seconds();
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakPC,S](float)
    {
        AMarau6PlayerController* PC=WeakPC.Get();
        const double Now=FPlatformTime::Seconds();
        if(!PC || Now-S->At>60){UE_LOG(LogTemp,Error,TEXT("MARAU6_VIEWMODEL aborted phase=%d"),S->Phase);return false;}
        auto* C=Cast<AMarau6Character>(PC->GetPawn());
        if(S->Phase==0)
        {
            if(!C || !C->HasEquippedRifle())return true;
            PC->SetIgnoreLookInput(true);PC->SetIgnoreMoveInput(true);S->Phase=3;S->At=Now;return true;
        }
        if(S->Shot>=UE_ARRAY_COUNT(Pitches)){UE_LOG(LogTemp,Display,TEXT("MARAU6_VIEWMODEL done"));return false;}
        if(S->Phase==3){PC->SetControlRotation(FRotator(Pitches[S->Shot],PC->GetControlRotation().Yaw,0));S->Phase=4;S->At=Now;return true;}
        if(Now-S->At<.6)return true;
        const FString File=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("LocalSettings")
            /FString::Printf(TEXT("viewmodel_%+03d.png"),FMath::RoundToInt(Pitches[S->Shot])));
        FScreenshotRequest::RequestScreenshot(File,false,false);
        UE_LOG(LogTemp,Display,TEXT("MARAU6_VIEWMODEL requested %s"),*File);
        ++S->Shot;S->Phase=3;S->At=Now;return true;
    }));
}
FAutoConsoleCommandWithWorld CaptureViewModelCommand(TEXT("Marau6.CaptureViewModel"),
    TEXT("Enter the raid as profile 1 and save first person view captures at several pitches."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&CaptureViewModel));
}
#endif
