// Copyright Epic Games, Inc. All Rights Reserved.

#include "Marau6GameMode.h"
#include "Engine/World.h"
#include "Marau6Persistence.h"
#include "Marau6PlayerController.h"

AMarau6GameMode::AMarau6GameMode()
{
	// stub
}

void AMarau6GameMode::Logout(AController* Exiting)
{
    UWorld* World=GetWorld();
    // PIE may destroy world subsystems before its final net-driver Logout callbacks.
    // Session teardown is recovered as abandonment from the journal next startup.
    if(IsValid(World) && !World->bIsTearingDown && UMarau6Persistence::UsesPersistentBase(World))
        if(auto* Bridge=World->GetSubsystem<UMarau6Persistence>())
            Bridge->Disconnect(Cast<AMarau6PlayerController>(Exiting));
    Super::Logout(Exiting);
}
