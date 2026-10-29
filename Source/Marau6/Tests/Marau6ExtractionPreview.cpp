#if WITH_EDITOR
#include "Marau6Character.h"
#include "Marau6ExtractionZone.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
namespace
{
void PreviewExtraction(UWorld* World)
{
    if (!World || !World->IsGameWorld()) return;
    APlayerController* PC = World->GetFirstPlayerController();
    auto* Pawn = PC ? Cast<AMarau6Character>(PC->GetPawn()) : nullptr;
    if (!Pawn) return;
    for (TActorIterator<AMarau6ExtractionZone> It(World); It; ++It)
    {
        UE_LOG(LogTemp, Display, TEXT("MARAU6_EXTRACTION tick=%d canTick=%d"), Pawn->IsActorTickEnabled(), Pawn->PrimaryActorTick.bCanEverTick);
        Pawn->SetActorLocation(It->GetActorLocation()+FVector(0,0,98));
        Pawn->GetCharacterMovement()->StopMovementImmediately();
        PC->SetControlRotation(FRotator(-35,225,0));
        PC->SetIgnoreMoveInput(true);
        UE_LOG(LogTemp, Display, TEXT("MARAU6_EXTRACTION pawn=%s zone=%s contained=%d"),
            *Pawn->GetActorLocation().ToString(), *It->GetActorLocation().ToString(), It->ContainsCharacter(Pawn));
        World->GetWorldSettings()->SetTimeDilation(.1f);
        break;
    }
}
FAutoConsoleCommandWithWorld PreviewExtractionCommand(TEXT("Marau6.PreviewExtraction"),
    TEXT("Temporary slowed extraction presentation preview; does not save configuration."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&PreviewExtraction));
}
#endif
