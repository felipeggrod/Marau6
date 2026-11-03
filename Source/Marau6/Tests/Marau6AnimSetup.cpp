#if WITH_EDITOR
#include "Marau6Character.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/BlendSpace.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "Templates/Function.h"

// Editor-only migration for project-owned copies. Template assets stay unchanged.
namespace Marau6AnimSetup
{
const TCHAR* TemplateLocomotion = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run.BS_Idle_Walk_Run");
const TCHAR* Folder = TEXT("/Game/Marau6/Anims");
const TCHAR* LocomotionName = TEXT("BS_Marau6_Locomotion");
const TCHAR* BodyPose = TEXT("/Game/Marau6/Anims/ABP_Marau6_TP_Rifle.ABP_Marau6_TP_Rifle");
// Players move at 5 m/s: the template only reaches its jog at 6 m/s, so normal
// movement read as a fast walk. Jog now lands on the move speed; aiming (3 m/s) still walks.
constexpr double JogSpeed = 500.0;

static bool Save(UObject* Asset)
{
    UPackage* Package = Asset->GetOutermost();
    const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(Package, Asset, *File, Args);
}

static UBlendSpace* Locomotion()
{
    const FString Path = FString(Folder) / LocomotionName;
    UBlendSpace* Space = FPackageName::DoesPackageExist(Path)
        ? LoadObject<UBlendSpace>(nullptr, *(Path + TEXT(".") + LocomotionName)) : nullptr;
    if (!Space)
    {
        UObject* Template = LoadObject<UObject>(nullptr, TemplateLocomotion);
        if (!Template) return nullptr;
        Space = Cast<UBlendSpace>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"))
            .Get().DuplicateAsset(LocomotionName, Folder, Template));
    }
    if (!Space) return nullptr;
    Space->Modify();
    // Samples snap to the speed grid: the template's 150 cm/s steps would pull 500 to 450.
    const FStructProperty* Axes = FindFProperty<FStructProperty>(UBlendSpace::StaticClass(), TEXT("BlendParameters"));
    if (!Axes) return nullptr;
    FBlendParameter* SpeedAxis = Axes->ContainerPtrToValuePtr<FBlendParameter>(Space, 1);
    SpeedAxis->GridNum = FMath::RoundToInt((SpeedAxis->Max - SpeedAxis->Min) / 100.f);
    int32 Moved = 0;
    for (int32 Index = 0; Index < Space->GetNumberOfBlendSamples(); ++Index)
    {
        // Jog row: the template's 600, or any earlier copy that snapped elsewhere above the walk.
        const FVector Value = Space->GetBlendSample(Index).SampleValue;
        if (Value.Y > 300.0 && !FMath::IsNearlyEqual(Value.Y, JogSpeed)
            && Space->EditSampleValue(Index, FVector(Value.X, JogSpeed, Value.Z))) ++Moved;
    }
    Space->ValidateSampleData();
    Space->ResampleData();
    Space->PostEditChange();
    UE_LOG(LogTemp, Display, TEXT("MARAU6_ANIM locomotion jog samples moved=%d"), Moved);
    return Save(Space) ? Space : nullptr;
}

template<typename T> static T* NodeValue(UEdGraphNode* Node, const TCHAR* Struct, const TCHAR* Field, FProperty*& Out)
{
    const FStructProperty* NodeStruct = FindFProperty<FStructProperty>(Node->GetClass(), Struct);
    if (!NodeStruct) return nullptr;
    Out = NodeStruct->Struct->FindPropertyByName(Field);
    return Out ? Out->ContainerPtrToValuePtr<T>(NodeStruct->ContainerPtrToValuePtr<void>(Node)) : nullptr;
}

/** Whether Pin's upstream chain has a node matching Test; tells the upper body branch from the legs. */
static bool Reaches(const UEdGraphPin* Pin, TFunctionRef<bool(UEdGraphNode*)> Test, TSet<const UEdGraphNode*>& Seen)
{
    for (const UEdGraphPin* Link : Pin->LinkedTo)
    {
        UEdGraphNode* Node = Link->GetOwningNode();
        if (Test(Node)) return true;
        if (Seen.Contains(Node)) continue;
        Seen.Add(Node);
        for (const UEdGraphPin* Input : Node->Pins)
            if (Input->Direction == EGPD_Input && Reaches(Input, Test, Seen)) return true;
    }
    return false;
}

static bool HasPose(UEdGraphNode* Node, const TCHAR* Sequence)
{
    if (Node->GetClass()->GetFName() != TEXT("AnimGraphNode_SequencePlayer")) return false;
    FProperty* Property = nullptr;
    const TObjectPtr<UObject>* Value = NodeValue<TObjectPtr<UObject>>(Node, TEXT("Node"), TEXT("Sequence"), Property);
    return Value && *Value && (*Value)->GetName() == Sequence;
}

static bool AdaptBody(UBlendSpace* Space)
{
    UAnimBlueprint* Blueprint = LoadObject<UAnimBlueprint>(nullptr, BodyPose);
    UClass* SlotClass = FindObject<UClass>(nullptr, TEXT("/Script/AnimGraph.AnimGraphNode_Slot"));
    if (!Blueprint || !SlotClass) return false;
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    for (UEdGraph* Graph : Graphs)
        for (UEdGraphNode* Node : Graph->Nodes)
            UE_LOG(LogTemp, Display, TEXT("MARAU6_ANIM node %s / %s"), *Graph->GetName(), *Node->GetClass()->GetName());
    UEdGraphNode* Legs = nullptr;
    int32 Swapped = 0, Slots = 0;
    for (UEdGraph* Graph : Graphs)
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node->GetClass() == SlotClass) ++Slots;
            if (Node->GetClass()->GetFName() != TEXT("AnimGraphNode_BlendSpacePlayer")) continue;
            FProperty* Property = nullptr;
            TObjectPtr<UBlendSpace>* Value = NodeValue<TObjectPtr<UBlendSpace>>(Node, TEXT("Node"), TEXT("BlendSpace"), Property);
            if (!Value) continue;
            const FString Current = *Value ? (*Value)->GetPathName() : FString();
            if (Current == TemplateLocomotion || *Value == Space)
            {
                Node->Modify();
                *Value = Space;
                // An exposed pin's default wins over the node property at compile time.
                if (UEdGraphPin* Pin = Node->FindPin(TEXT("BlendSpace"))) Pin->DefaultObject = Space;
                Legs = Node;
                ++Swapped;
            }
        }
    if (!Legs) { UE_LOG(LogTemp, Error, TEXT("MARAU6_ANIM locomotion node not found")); return false; }

    // Reload plays on the upper body only: insert a slot in the layered blend's
    // branch that does not carry locomotion, so the legs keep running.
    if (Slots == 0)
    {
        for (UEdGraph* Graph : Graphs)
            for (UEdGraphNode* Node : TArray<UEdGraphNode*>(Graph->Nodes))
            {
                if (Node->GetClass()->GetFName() != TEXT("AnimGraphNode_LayeredBoneBlend")) continue;
                UEdGraphPin* Base = Node->FindPin(TEXT("BasePose"));
                UEdGraphPin* Upper = Node->FindPin(TEXT("BlendPoses_0"));
                if (!Base || !Upper || Upper->LinkedTo.Num() != 1) continue;
                auto IsLegs = [Legs](UEdGraphNode* Candidate)
                    { return Candidate == Legs || Candidate->GetClass()->GetName().Contains(TEXT("StateMachine")); };
                auto IsRifle = [](UEdGraphNode* Candidate) { return HasPose(Candidate, TEXT("MF_Rifle_Idle_ADS")); };
                TSet<const UEdGraphNode*> Seen;
                const bool bUpperLegs = Reaches(Upper, IsLegs, Seen);
                Seen.Reset();
                const bool bBaseLegs = Reaches(Base, IsLegs, Seen);
                Seen.Reset();
                const bool bUpperRifle = Reaches(Upper, IsRifle, Seen);
                UE_LOG(LogTemp, Display, TEXT("MARAU6_ANIM layered blend base legs=%d upper legs=%d upper rifle=%d"),
                    bBaseLegs, bUpperLegs, bUpperRifle);
                if (bUpperLegs || !(bBaseLegs || bUpperRifle)) continue;
                UEdGraphPin* Source = Upper->LinkedTo[0];
                UEdGraphNode* Slot = NewObject<UEdGraphNode>(Graph, SlotClass, NAME_None, RF_Transactional);
                FProperty* Property = nullptr;
                if (FName* SlotName = NodeValue<FName>(Slot, TEXT("Node"), TEXT("SlotName"), Property)) *SlotName = AMarau6Character::ReloadSlot;
                Slot->CreateNewGuid();
                Slot->NodePosX = Node->NodePosX - 260;
                Slot->NodePosY = Node->NodePosY + 160;
                Graph->AddNode(Slot, false, false);
                Slot->AllocateDefaultPins();
                UEdGraphPin* SlotIn = Slot->FindPin(TEXT("Source"));
                UEdGraphPin* SlotOut = nullptr;
                for (UEdGraphPin* Pin : Slot->Pins) if (Pin->Direction == EGPD_Output) SlotOut = Pin;
                const UEdGraphSchema* Schema = Graph->GetSchema();
                if (!SlotIn || !SlotOut || !Schema) return false;
                Upper->BreakLinkTo(Source);
                if (!Schema->TryCreateConnection(Source, SlotIn) || !Schema->TryCreateConnection(SlotOut, Upper))
                {
                    UE_LOG(LogTemp, Error, TEXT("MARAU6_ANIM reload slot wiring failed"));
                    return false;
                }
                ++Slots;
            }
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    const bool bSaved = Blueprint->Status != BS_Error && Save(Blueprint);
    UE_LOG(LogTemp, Display, TEXT("MARAU6_ANIM body saved=%d locomotion=%d slots=%d"), bSaved, Swapped, Slots);
    return bSaved && Slots == 1;
}

static void Setup()
{
    UBlendSpace* Space = Locomotion();
    if (!Space || !AdaptBody(Space))
        UE_LOG(LogTemp, Error, TEXT("MARAU6_ANIM setup failed; template assets remain unchanged."));
}
static FAutoConsoleCommand SetupCommand(TEXT("Marau6.SetupLocomotion"),
    TEXT("Running locomotion copy and the upper body reload slot for the third person pose."),
    FConsoleCommandDelegate::CreateStatic(&Setup));
}
#endif
