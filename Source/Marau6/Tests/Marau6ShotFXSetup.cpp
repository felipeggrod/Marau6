#if WITH_EDITOR
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionDistance.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionPower.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionTransform.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "IAssetTools.h"
#include "Sound/SoundWave.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

// Editor-only generator for the assets used by AMarau6ShotFX. Materials are built
// from engine math nodes and the rifle report is imported from the WAV written by
// scripts/Generate-RifleShot.py, so no external content is needed.
namespace Marau6ShotFXAssets
{
template<typename T> T* Add(UMaterial* M)
{
    T* E = NewObject<T>(M);
    M->GetExpressionCollection().AddExpression(E);
    return E;
}
UMaterialExpression* Mul(UMaterial* M, UMaterialExpression* A, UMaterialExpression* B)
{
    auto* E = Add<UMaterialExpressionMultiply>(M);
    E->A.Connect(0, A);
    E->B.Connect(0, B);
    return E;
}
UMaterialExpression* Scalar(UMaterial* M, const TCHAR* Name, float Value)
{
    auto* E = Add<UMaterialExpressionScalarParameter>(M);
    E->ParameterName = Name;
    E->DefaultValue = Value;
    return E;
}
UMaterialExpression* Color(UMaterial* M, FLinearColor Value)
{
    auto* E = Add<UMaterialExpressionVectorParameter>(M);
    E->ParameterName = TEXT("Color");
    E->DefaultValue = Value;
    return E;
}
// Soft round falloff from the centre of the UV square: (1 - 2 * distance)^2.
UMaterialExpression* RadialMask(UMaterial* M)
{
    auto* Centre = Add<UMaterialExpressionConstant2Vector>(M);
    Centre->R = .5f;
    Centre->G = .5f;
    auto* Distance = Add<UMaterialExpressionDistance>(M);
    Distance->A.Connect(0, Add<UMaterialExpressionTextureCoordinate>(M));
    Distance->B.Connect(0, Centre);
    auto* Two = Add<UMaterialExpressionConstant>(M);
    Two->R = 2.f;
    auto* Inverse = Add<UMaterialExpressionOneMinus>(M);
    Inverse->Input.Connect(0, Mul(M, Distance, Two));
    auto* Clamp = Add<UMaterialExpressionSaturate>(M);
    Clamp->Input.Connect(0, Inverse);
    auto* Curve = Add<UMaterialExpressionPower>(M);
    Curve->Base.Connect(0, Clamp);
    Curve->ConstExponent = 2.f;
    return Curve;
}
// Opaque where a tube faces the viewer, fading toward its silhouette. The end caps
// (normals along the tube's local Z) vanish, so a trail seen end-on shows no disc.
UMaterialExpression* TubeMask(UMaterial* M, float Exponent)
{
    auto* Edge = Add<UMaterialExpressionFresnel>(M);
    Edge->Exponent = Exponent;
    Edge->BaseReflectFraction = 0.f;
    auto* Inverse = Add<UMaterialExpressionOneMinus>(M);
    Inverse->Input.Connect(0, Edge);
    auto* Up = Add<UMaterialExpressionConstant3Vector>(M);
    Up->Constant = FLinearColor(0.f, 0.f, 1.f);
    auto* Axis = Add<UMaterialExpressionTransform>(M);
    Axis->TransformSourceType = TRANSFORMSOURCE_Local;
    Axis->TransformType = TRANSFORM_World;
    Axis->Input.Connect(0, Up);
    auto* Along = Add<UMaterialExpressionDotProduct>(M);
    Along->A.Connect(0, Axis);
    Along->B.Connect(0, Add<UMaterialExpressionVertexNormalWS>(M));
    auto* Facing = Add<UMaterialExpressionAbs>(M);
    Facing->Input.Connect(0, Along);
    auto* Side = Add<UMaterialExpressionOneMinus>(M);
    Side->Input.Connect(0, Facing);
    auto* Sharp = Add<UMaterialExpressionPower>(M);
    Sharp->Base.Connect(0, Side);
    Sharp->ConstExponent = 4.f;
    return Mul(M, Inverse, Sharp);
}

bool Save(UMaterial* M)
{
    M->PreEditChange(nullptr);
    M->PostEditChange();
    FAssetRegistryModule::AssetCreated(M);
    UPackage* Package = M->GetOutermost();
    Package->MarkPackageDirty();
    const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    const bool bSaved = UPackage::SavePackage(Package, M, *File, Args);
    UE_LOG(LogTemp, Display, TEXT("MARAU6_SHOTFX %s saved=%d"), *M->GetName(), bSaved);
    return bSaved;
}
bool SavePackageOf(UObject* Asset)
{
    UPackage* Package = Asset->GetOutermost();
    Package->MarkPackageDirty();
    const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    const bool bSaved = UPackage::SavePackage(Package, Asset, *File, Args);
    UE_LOG(LogTemp, Display, TEXT("MARAU6_SHOTFX %s saved=%d"), *Asset->GetName(), bSaved);
    return bSaved;
}
UMaterial* Create(const TCHAR* Name, EBlendMode Blend)
{
    // Re-runs replace the asset in place, so an existing package must be fully loaded to save.
    const FString Path = FString::Printf(TEXT("/Game/Marau6/Effects/%s"), Name);
    UPackage* Package = FPackageName::DoesPackageExist(Path) ? LoadPackage(nullptr, *Path, LOAD_None) : nullptr;
    if (!Package) Package = CreatePackage(*Path);
    Package->FullyLoad();
    if (UMaterial* Old = FindObject<UMaterial>(Package, Name))
        Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
    UMaterial* M = NewObject<UMaterial>(Package, Name, RF_Public | RF_Standalone);
    M->BlendMode = Blend;
    M->SetShadingModel(MSM_Unlit);
    M->TwoSided = true;
    M->bUsedWithStaticLighting = false;
    return M;
}

// Bullet holes and blood: deferred decals whose opacity is a radial falloff,
// hardened by Sharpness and broken up by world-space noise.
UMaterial* Decal(const TCHAR* Name, FLinearColor Tint, float Roughness, float Sharpness, float NoiseScale)
{
    UMaterial* M = Create(Name, BLEND_Translucent);
    M->MaterialDomain = MD_DeferredDecal;
    M->SetShadingModel(MSM_DefaultLit);
    M->TwoSided = false;
    auto* Rough = Add<UMaterialExpressionConstant>(M);
    Rough->R = Roughness;
    auto* Edge = Add<UMaterialExpressionNoise>(M);
    Edge->Scale = NoiseScale;
    Edge->Levels = 2;
    Edge->OutputMin = .55f;
    Edge->OutputMax = 1.f;
    auto* Hard = Add<UMaterialExpressionConstant>(M);
    Hard->R = Sharpness;
    auto* Clamp = Add<UMaterialExpressionSaturate>(M);
    Clamp->Input.Connect(0, Mul(M, Mul(M, RadialMask(M), Hard), Edge));
    M->GetEditorOnlyData()->BaseColor.Connect(0, Color(M, Tint));
    M->GetEditorOnlyData()->Roughness.Connect(0, Rough);
    M->GetEditorOnlyData()->Opacity.Connect(0, Clamp);
    return M;
}

bool ImportReport()
{
    const FString Wav = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SourceAudio/S_RifleShot.wav"));
    if (!FPaths::FileExists(Wav))
    {
        UE_LOG(LogTemp, Error, TEXT("MARAU6_SHOTFX missing %s; run scripts/Generate-RifleShot.py first."), *Wav);
        return false;
    }
    UAutomatedAssetImportData* Import = NewObject<UAutomatedAssetImportData>();
    Import->Filenames.Add(Wav);
    Import->DestinationPath = TEXT("/Game/Marau6/Audio");
    Import->bReplaceExisting = true;
    const TArray<UObject*> Imported = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().ImportAssetsAutomated(Import);
    return Imported.Num() == 1 && Cast<USoundWave>(Imported[0]) && SavePackageOf(Imported[0]);
}

void Setup()
{
    UMaterial* Flash = Create(TEXT("M_MuzzleFlash"), BLEND_Additive);
    Flash->GetEditorOnlyData()->EmissiveColor.Connect(0,
        Mul(Flash, Mul(Flash, Color(Flash, FLinearColor(1.f, .55f, .2f)), Scalar(Flash, TEXT("Intensity"), 30.f)), RadialMask(Flash)));

    UMaterial* Tracer = Create(TEXT("M_Tracer"), BLEND_Additive);
    Tracer->GetEditorOnlyData()->EmissiveColor.Connect(0,
        Mul(Tracer, Mul(Tracer, Color(Tracer, FLinearColor(1.f, .7f, .35f)), Scalar(Tracer, TEXT("Intensity"), 25.f)), TubeMask(Tracer, 2.f)));

    // Smoke breaks up through world-space noise, so a drifting trail looks wispy.
    UMaterial* Smoke = Create(TEXT("M_ShotSmoke"), BLEND_Translucent);
    auto* Wisps = Add<UMaterialExpressionNoise>(Smoke);
    Wisps->Scale = .04f;
    Wisps->Levels = 3;
    Wisps->bTurbulence = true;
    Wisps->OutputMin = .15f;
    Wisps->OutputMax = 1.f;
    Smoke->GetEditorOnlyData()->EmissiveColor.Connect(0, Color(Smoke, FLinearColor(.55f, .55f, .55f)));
    Smoke->GetEditorOnlyData()->Opacity.Connect(0,
        Mul(Smoke, Mul(Smoke, Scalar(Smoke, TEXT("Opacity"), .3f), TubeMask(Smoke, 1.5f)), Wisps));

    // Impact dust and blood mist: a soft translucent ball tinted per shot.
    UMaterial* Puff = Create(TEXT("M_ImpactPuff"), BLEND_Translucent);
    auto* Breakup = Add<UMaterialExpressionNoise>(Puff);
    Breakup->Scale = .08f;
    Breakup->Levels = 3;
    Breakup->OutputMin = .3f;
    Breakup->OutputMax = 1.f;
    auto* Soft = Add<UMaterialExpressionFresnel>(Puff);
    Soft->Exponent = 1.2f;
    Soft->BaseReflectFraction = 0.f;
    auto* Core = Add<UMaterialExpressionOneMinus>(Puff);
    Core->Input.Connect(0, Soft);
    Puff->GetEditorOnlyData()->EmissiveColor.Connect(0, Color(Puff, FLinearColor(.45f, .42f, .38f)));
    Puff->GetEditorOnlyData()->Opacity.Connect(0, Mul(Puff, Mul(Puff, Scalar(Puff, TEXT("Opacity"), .5f), Core), Breakup));

    UMaterial* Hole = Decal(TEXT("M_BulletHole"), FLinearColor(.015f, .013f, .012f), .9f, 3.f, .6f);
    UMaterial* Blood = Decal(TEXT("M_BloodDecal"), FLinearColor(.12f, 0.f, 0.f), .25f, 2.2f, .15f);

    bool bSaved = true;
    for (UMaterial* M : { Flash, Tracer, Smoke, Puff, Hole, Blood }) bSaved &= Save(M);
    if (!bSaved || !ImportReport())
        UE_LOG(LogTemp, Error, TEXT("MARAU6_SHOTFX asset generation failed."));
}
static FAutoConsoleCommand SetupCommand(TEXT("Marau6.SetupShotFX"),
    TEXT("Create or replace the shot materials in /Game/Marau6/Effects and import the rifle report."),
    FConsoleCommandDelegate::CreateStatic(&Setup));
}
#endif
