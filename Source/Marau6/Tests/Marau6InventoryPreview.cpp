#if WITH_EDITOR
#include "Marau6Character.h"
#include "Marau6InventoryComponent.h"
#include "Marau6LootActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
namespace
{
void PreviewInventory(UWorld* World)
{
    if(!World || !World->IsGameWorld()) return;
    auto* PC=World->GetFirstPlayerController();auto* C=PC?Cast<AMarau6Character>(PC->GetPawn()):nullptr;
    if(!C) return;
    for(TActorIterator<AMarau6LootActor> It(World);It;++It)
    {
        if(It->GetItem().Kind!=EMarau6ItemKind::Supply) continue;
        C->SetActorLocation(It->GetActorLocation()+FVector(-130,0,71));
        C->GetCharacterMovement()->StopMovementImmediately();
        PC->SetControlRotation((It->GetActorLocation()-C->GetPawnViewLocation()).Rotation());
        PC->SetIgnoreMoveInput(true);C->OpenInventoryResult();
        UE_LOG(LogTemp,Display,TEXT("MARAU6_INVENTORY_PREVIEW target=%s items=%d"),*It->GetActorLocation().ToString(),C->GetInventory()->GetSnapshot().Items.Num());break;
    }
}
FAutoConsoleCommandWithWorld PreviewInventoryCommand(TEXT("Marau6.PreviewInventory"),
    TEXT("Temporary inventory interaction preview; does not save map or configuration."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&PreviewInventory));
}
#endif
