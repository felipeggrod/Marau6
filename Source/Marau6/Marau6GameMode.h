// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Marau6GameMode.generated.h"

/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class AMarau6GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMarau6GameMode();
    virtual void Logout(AController* Exiting) override;
};



