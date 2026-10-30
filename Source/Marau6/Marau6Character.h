// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPtr.h"
#include "Marau6Character.generated.h"

class UMarau6InventoryComponent;
class UMarau6ItemLedger;
class AMarau6ExtractionZone;
class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UAnimInstance;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UENUM(BlueprintType)
enum class EMarau6RaidState : uint8 { Active, Extracting, Extracted, Eliminated };

USTRUCT()
struct FMarau6RaidStatus
{
    GENERATED_BODY()
    UPROPERTY() EMarau6RaidState State = EMarau6RaidState::Active;
    UPROPERTY() double ExtractionEndServerTime = 0.0;
};

/** One replicated snapshot keeps the owner's counters and reload state together. */
USTRUCT(BlueprintType)
struct FMarau6AmmoState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    int32 Magazine = 10;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    int32 Reserve = 30;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    bool bReloading = false;
};

/**
 *  A basic first person character
 */
UCLASS(abstract)
class AMarau6Character : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

    /** Presentation only: neither mesh participates in damage or collision. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presentation", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USkeletalMeshComponent> FirstPersonRifle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presentation", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USkeletalMeshComponent> ThirdPersonRifle;

    UPROPERTY(EditDefaultsOnly, Category = "Presentation")
    TSoftClassPtr<UAnimInstance> PrototypeFirstPersonPose;

    UPROPERTY(EditDefaultsOnly, Category = "Presentation")
    TSoftClassPtr<UAnimInstance> PrototypeThirdPersonPose;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

protected:

	/** Vida atual, controlada pelo servidor. */
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float Health = 100.0f;

	/** Atualiza o cliente quando a vida muda. */
	UFUNCTION()
	void OnRep_Health();


    /** Runtime mapping for the prototype; existing movement assets stay unchanged. */
    UPROPERTY(Transient)
    TObjectPtr<UInputAction> PrototypeFireAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> PrototypeReloadAction;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    FMarau6AmmoState AmmoState;

    FTimerHandle ReloadTimer;


    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> PrototypeFireMapping;

    void DoFire();
    void DoReload();

    UFUNCTION(Server, Reliable)
    void ServerReload();

    void FinishReload();
    void CancelReload();

    UFUNCTION(Server, Unreliable)
    void ServerFire();

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastShot(FVector_NetQuantize Point, FVector_NetQuantizeNormal Normal, bool bBlockingHit, bool bHitCharacter);

    friend class UMarau6ItemLedger;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UMarau6InventoryComponent> Inventory;
    bool bInventoryOpen=false;
    int32 SelectedInventorySlot=0;
    void ToggleInventory();
    void NextInventorySlot();
    void PreviousInventorySlot();
    void PickupLoot();
    void DropSelectedItem();
    void EquipSelectedItem();
    void ProtectSelectedItem();
    virtual void PostInitializeComponents() override;
    UPROPERTY(ReplicatedUsing=OnRep_RaidStatus)
    FMarau6RaidStatus RaidStatus;
    TWeakObjectPtr<AMarau6ExtractionZone> ExtractionZone;
    UFUNCTION() void OnRep_RaidStatus();
    void UpdateExtraction();
    void CompleteExtraction();
    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Server-owned cooldown, never accepted from a client. */
    double LastServerFireTime = -1.0;

    static constexpr int32 PrototypeMagazineCapacity = 10;
    static constexpr float PrototypeReloadDuration = 2.0f;

    static constexpr float PrototypeDamage = 25.0f;
    static constexpr float PrototypeRange = 10000.0f;
    static constexpr double PrototypeFireInterval = 0.25;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;
	
public:
	AMarau6Character();
    virtual void Tick(float DeltaSeconds) override;
    UMarau6InventoryComponent* GetInventory() const { return Inventory; }
    bool HasEquippedRifle() const;
    bool IsInventoryOpen() const { return bInventoryOpen; }
    int32 GetSelectedInventorySlot() const { return SelectedInventorySlot; }
    void OpenInventoryResult() { bInventoryOpen=true; }
    void OnInventoryChanged(bool bEquipmentChanged);
    EMarau6RaidState GetRaidState() const { return RaidStatus.State; }
    bool CanParticipate() const { return IsAlive() && (RaidStatus.State == EMarau6RaidState::Active
        || RaidStatus.State == EMarau6RaidState::Extracting); }
    float GetExtractionSecondsRemaining() const;
    
    /** Current health is read-only outside the server damage handler. */
    UFUNCTION(BlueprintPure, Category = "Health")
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure, Category = "Health")
    bool IsAlive() const { return Health > 0.0f; }

    UFUNCTION(BlueprintPure, Category = "Combat")
    int32 GetMagazineAmmo() const { return AmmoState.Magazine; }

    UFUNCTION(BlueprintPure, Category = "Combat")
    int32 GetReserveAmmo() const { return AmmoState.Reserve; }

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool IsReloading() const { return AmmoState.bReloading; }

    virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;


	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

