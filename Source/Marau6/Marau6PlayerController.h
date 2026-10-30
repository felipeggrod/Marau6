// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Marau6PlayerController.generated.h"

class UMarau6Persistence;
class UInputMappingContext;
class UUserWidget;

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 */
UCLASS(abstract, config="Game")
class MARAU6_API AMarau6PlayerController : public APlayerController
{
	GENERATED_BODY()
    friend class FMarau6InventoryReorderTest;
	
public:

	/** Constructor */
	AMarau6PlayerController();

    bool IsUsingBase() const { return bUseBase; }
    bool IsInRaid() const { return bInRaid; }
    const FString& GetBaseJson() const { return BaseJson; }
    const FString& GetBaseMessage() const { return BaseMessage; }
    const FString& GetBaseBackendStatus() const { return BaseBackendStatus; }
    int32 GetBaseProfile() const { return BaseProfile; }
    int32 GetBaseSelection() const { return BaseSelection; }
    int32 GetBaseSource() const { return BaseSource; }
    static int32 BaseSlot(int32 Index) { return Index<11?Index:100+Index-11; }
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void OnPossess(APawn* NewPawn) override;
    UFUNCTION(Server,Reliable) void ServerSelectProfile(int32 Number);
    UFUNCTION(Server,Reliable) void ServerBaseMove(int32 Source,int32 Destination);
    UFUNCTION(Server,Reliable) void ServerEnterRaid();
    UFUNCTION(Server,Reliable) void ServerNextRound();
private:
    friend class UMarau6Persistence;
    UPROPERTY(Replicated) bool bUseBase=false;
    UPROPERTY(Replicated) bool bInRaid=false;
    bool bBaseBusy=false;
    UPROPERTY(Replicated) int32 BaseProfile=0;
    UPROPERTY(Replicated) FString BaseJson;
    UPROPERTY(Replicated) FString BaseMessage;
    UPROPERTY(Replicated) FString BaseBackendStatus;
    int32 BaseSelection=0,BaseSource=INDEX_NONE;
    void SelectOne();void SelectTwo();void BaseUp();void BaseDown();void BaseLeft();void BaseRight();void BaseAccept();void BaseEnter();void BaseNext();
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;
};
