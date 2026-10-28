// Copyright Epic Games, Inc. All Rights Reserved.


#include "Marau6PlayerController.h"
#include "Marau6PrototypeHUD.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Marau6CameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Marau6.h"
#include "Widgets/Input/SVirtualJoystick.h"

AMarau6PlayerController::AMarau6PlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AMarau6CameraManager::StaticClass();
}

void AMarau6PlayerController::BeginPlay()
{
	Super::BeginPlay();

    if (IsLocalPlayerController())
    {
        ClientSetHUD(AMarau6PrototypeHUD::StaticClass());
    }

	
	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogMarau6, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AMarau6PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
	
}

bool AMarau6PlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
