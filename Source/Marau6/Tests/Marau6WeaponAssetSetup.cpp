#if WITH_EDITOR
#include "Marau6Character.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Variable.h"
#include "K2Node_VariableGet.h"
#include "GameFramework/Controller.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "UObject/UnrealType.h"

// Editor-only migration for project-owned copies. It never edits template assets.
namespace Marau6WeaponAssets
{
static bool AdaptPose(const TCHAR* Source, const FString& Name)
{
    const FString Folder = TEXT("/Game/Marau6/Anims");
    const FString Destination = Folder + TEXT("/") + Name;
    UBlueprint* Blueprint = nullptr;
    if (FPackageName::DoesPackageExist(Destination))
    {
        Blueprint = LoadObject<UBlueprint>(nullptr, *Destination);
    }
    else
    {
        UObject* Original = LoadObject<UObject>(nullptr, Source);
        if (!Original) return false;
        Blueprint = Cast<UBlueprint>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(
            TEXT("AssetTools")).Get().DuplicateAsset(Name, Folder, Original));
    }
    if (!Blueprint) return false;
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    int32 CastsChanged = 0;
    int32 AimCallsChanged = 0;
    for (UEdGraph* Graph : Graphs)
    {
        const auto Nodes = Graph->Nodes;
        const UEdGraphSchema_K2* Schema = Cast<UEdGraphSchema_K2>(Graph->GetSchema());
        if (!Schema) continue;
        // The template computes normalized pitch but leaves the aim-offset Y
        // pin unconnected. Feed that value into the project-owned rifle pose.
        if (Name == TEXT("ABP_Marau6_TP_Rifle"))
        {
            for (UEdGraphNode* Node : Nodes)
            {
                if (Node->GetClass()->GetFName() != TEXT("AnimGraphNode_RotationOffsetBlendSpace")) continue;
                UEdGraphPin* PitchInput = Node->FindPin(TEXT("Y"));
                if (!PitchInput || !PitchInput->LinkedTo.IsEmpty()) continue;
                UK2Node_VariableGet* Pitch = NewObject<UK2Node_VariableGet>(Graph);
                Pitch->VariableReference.SetSelfMember(TEXT("PitchN"));
                Pitch->CreateNewGuid();
                Pitch->NodePosX = Node->NodePosX - 220;
                Pitch->NodePosY = Node->NodePosY;
                Graph->AddNode(Pitch, false, false);
                Pitch->AllocateDefaultPins();
                if (!Schema->TryCreateConnection(Pitch->GetValuePin(), PitchInput)) return false;
            }
        }
        // A remote pawn has no local controller. Since aim now comes from the pawn,
        // the graph's validity guard must validate that pawn as well.
        for (UEdGraphNode* Node : Nodes)
        {
            UK2Node_CallFunction* ControllerCall = Cast<UK2Node_CallFunction>(Node);
            if (!ControllerCall || !ControllerCall->GetTargetFunction()
                || ControllerCall->GetTargetFunction()->GetFName() != TEXT("GetController")) continue;
            UEdGraphPin* PawnInput = ControllerCall->FindPin(UEdGraphSchema_K2::PN_Self);
            UEdGraphPin* ControllerResult = ControllerCall->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
            if (!PawnInput || PawnInput->LinkedTo.Num() != 1 || !ControllerResult) continue;
            const TArray<UEdGraphPin*> Consumers = ControllerResult->LinkedTo;
            for (UEdGraphPin* Consumer : Consumers)
            {
                UE_LOG(LogTemp, Display, TEXT("MARAU6_POSE guard consumer %s %s %s"),
                    *Name, *Consumer->GetOwningNode()->GetClass()->GetName(), *Consumer->PinName.ToString());
                if (Consumer->PinName == TEXT("InputObject"))
                {
                    ControllerResult->BreakLinkTo(Consumer);
                    if (!Schema->TryCreateConnection(PawnInput->LinkedTo[0], Consumer)) return false;
                    UE_LOG(LogTemp, Display, TEXT("MARAU6_POSE pawn validity guard repaired %s"), *Name);
                }
            }
        }

        // Widen inherited component getters before reconnecting the broader cast.
        for (UEdGraphNode* Node : Nodes)
        {
            UK2Node_Variable* Variable = Cast<UK2Node_Variable>(Node);
            if (Variable && Variable->VariableReference.GetMemberParentClass()
                && Variable->VariableReference.GetMemberParentClass()->GetName() == TEXT("ShooterCharacter"))
            {
                const FName Member = Variable->VariableReference.GetMemberName();
                if (FindFProperty<FProperty>(AMarau6Character::StaticClass(), Member))
                {
                    Variable->VariableReference.SetExternalMember(Member, AMarau6Character::StaticClass());
                    Variable->ReconstructNode();
                }
            }
        }
        for (UEdGraphNode* Node : Nodes)
        {
            if (UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(Node))
            {
                if (CastNode->TargetType && CastNode->TargetType->GetName() == TEXT("ShooterCharacter"))
                {
                    UEdGraphPin* PreviousResult = CastNode->GetCastResultPin();
                    const TArray<UEdGraphPin*> Links = PreviousResult->LinkedTo;
                    CastNode->Modify();
                    CastNode->TargetType = AMarau6Character::StaticClass();
                    CastNode->ReconstructNode();
                    UEdGraphPin* Result = CastNode->GetCastResultPin();
                    for (UEdGraphPin* Link : Links)
                    {
                        if (!Schema->TryCreateConnection(Result, Link))
                        {
                            UE_LOG(LogTemp, Error, TEXT("MARAU6_POSE cast rewire failed: %s"), *Name);
                            return false;
                        }
                    }
                    ++CastsChanged;
                }
            }
            UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
            UFunction* Function = Call ? Call->GetTargetFunction() : nullptr;
            if (!Function || Function->GetFName() != TEXT("GetControlRotation")) continue;
            UEdGraphPin* OldSelf = Call->FindPin(UEdGraphSchema_K2::PN_Self);
            UEdGraphPin* PawnSource = nullptr;
            if (Function->GetOwnerClass() == AController::StaticClass())
            {
                if (!OldSelf || OldSelf->LinkedTo.Num() != 1) return false;
                UK2Node_CallFunction* GetController = Cast<UK2Node_CallFunction>(
                    OldSelf->LinkedTo[0]->GetOwningNode());
                if (!GetController || !GetController->GetTargetFunction()
                    || GetController->GetTargetFunction()->GetFName() != TEXT("GetController")) return false;
                UEdGraphPin* PawnInput = GetController->FindPin(UEdGraphSchema_K2::PN_Self);
                if (!PawnInput || PawnInput->LinkedTo.Num() != 1) return false;
                PawnSource = PawnInput->LinkedTo[0];
            }
            else if (Function->GetOwnerClass() == APawn::StaticClass())
            {
                if (!OldSelf || OldSelf->LinkedTo.Num() != 1) return false;
                PawnSource = OldSelf->LinkedTo[0];
            }
            else continue;

            UK2Node_CallFunction* Aim = NewObject<UK2Node_CallFunction>(Graph);
            Aim->SetFromFunction(APawn::StaticClass()->FindFunctionByName(
                GET_FUNCTION_NAME_CHECKED(APawn, GetBaseAimRotation)));
            Aim->CreateNewGuid();
            Aim->NodePosX = Call->NodePosX;
            Aim->NodePosY = Call->NodePosY;
            Graph->AddNode(Aim, false, false);
            Aim->AllocateDefaultPins();
            if (!Schema->TryCreateConnection(PawnSource, Aim->FindPin(UEdGraphSchema_K2::PN_Self)))
                return false;
            UEdGraphPin* OldReturn = Call->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
            UEdGraphPin* NewReturn = Aim->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
            if (!OldReturn || !NewReturn || OldReturn->SubPins.Num() != 0) return false;
            const TArray<UEdGraphPin*> Links = OldReturn->LinkedTo;
            OldReturn->BreakAllPinLinks();
            for (UEdGraphPin* Link : Links)
            {
                if (!Schema->TryCreateConnection(NewReturn, Link)) return false;
            }
            FBlueprintEditorUtils::RemoveNode(Blueprint, Call, true);
            ++AimCallsChanged;
        }
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error) return false;
    UPackage* Package = Blueprint->GetOutermost();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    const bool Saved = UPackage::SavePackage(Package, Blueprint, *Filename, Args);
    UE_LOG(LogTemp, Display, TEXT("MARAU6_POSE %s saved=%d casts=%d aim=%d"),
        *Name, Saved, CastsChanged, AimCallsChanged);
    return Saved;
}
static void Setup()
{
    const bool FP = AdaptPose(TEXT("/Game/Variant_Shooter/Anims/ABP_FP_Weapon"), TEXT("ABP_Marau6_FP_Rifle"));
    const bool TP = AdaptPose(TEXT("/Game/Variant_Shooter/Anims/ABP_TP_Rifle"), TEXT("ABP_Marau6_TP_Rifle"));
    if (!FP || !TP)
        UE_LOG(LogTemp, Error, TEXT("MARAU6_POSE adaptation failed; template assets remain unchanged."));
}
static FAutoConsoleCommand SetupCommand(TEXT("Marau6.SetupWeaponVisuals"),
    TEXT("Create or refresh project pose copies for the prototype rifle."),
    FConsoleCommandDelegate::CreateStatic(&Setup));

static void Preview(UWorld* World)
{
    if (!World || !World->IsGameWorld()) return;
    APlayerController* PC = World->GetFirstPlayerController();
    AMarau6Character* Viewer = PC ? Cast<AMarau6Character>(PC->GetPawn()) : nullptr;
    if (!Viewer) return;
    const FRotator Facing(0.0, Viewer->GetActorRotation().Yaw, 0.0);
    PC->SetControlRotation(Facing);
    PC->SetIgnoreLookInput(true);
    PC->SetIgnoreMoveInput(true);
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AMarau6Character* Other = World->SpawnActor<AMarau6Character>(
        Viewer->GetClass(), Viewer->GetActorLocation() + Facing.Vector() * 350.0,
        FRotator(0.0, Facing.Yaw + 180.0, 0.0), Spawn);
    if (!Other) return;
    Other->GetCharacterMovement()->DisableMovement();
    Other->SetRemoteViewPitch(40.0);
    TWeakObjectPtr<AMarau6Character> WeakOther(Other);
    FTimerHandle Handle;
    World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([WeakOther]()
    {
        if (!WeakOther.IsValid()) return;
        UAnimInstance* Pose = WeakOther->GetMesh()->GetAnimInstance();
        UE_LOG(LogTemp, Display, TEXT("MARAU6_PREVIEW aim=%s grip=%s"),
            *WeakOther->GetBaseAimRotation().ToString(),
            *WeakOther->GetMesh()->GetSocketRotation(TEXT("HandGrip_R")).ToString());
        for (TFieldIterator<FProperty> It(Pose->GetClass()); It; ++It)
        {
            if (!It->GetName().Contains(TEXT("Pitch")) && !It->GetName().Contains(TEXT("Aim"))) continue;
            FString Value;
            It->ExportTextItem_Direct(Value, It->ContainerPtrToValuePtr<void>(Pose), nullptr, Pose, PPF_None);
            UE_LOG(LogTemp, Display, TEXT("MARAU6_PREVIEW %s=%s"), *It->GetName(), *Value.Left(500));
        }
    }), 3.0f, false);
}
static FAutoConsoleCommandWithWorld PreviewCommand(TEXT("Marau6.PreviewWeapon"),
    TEXT("Temporary fixed-camera local presentation preview; does not save the map."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&Preview));
}
#endif
