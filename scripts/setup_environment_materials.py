"""Build the Poly Haven environment materials and put them on the test maps.

Grey prototype grid  -> MI_Marau6_Stone          (Poly Haven rock_tile_floor)
Orange colorway      -> MI_Marau6_ScratchedMetal (Poly Haven blue_metal_plate)

The LevelPrototyping/template assets are not touched: only the material slots of the
level actors are overridden. Run with UnrealEditor-Cmd -run=pythonscript -script=<abs path>.
Set the environment variable MARAU6_ENV_DUMP=1 to only list the materials in use.
"""

from pathlib import Path
import os
import unreal


PROJECT = Path(unreal.Paths.project_dir())
SOURCE_DIR = PROJECT / 'SourceArt' / 'Environment'
ASSET_DIR = '/Game/Marau6/Environment'
MAPS = ['/Game/FirstPerson/Lvl_FirstPerson', '/Game/Variant_Shooter/Lvl_Shooter']
DUMP_ONLY = os.environ.get('MARAU6_ENV_DUMP') == '1'

lib = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# name -> (source file, kind)
TEXTURES = {
    'T_Stone_D': ('rock_tile_floor/rock_tile_floor_diff_2k.jpg', 'color'),
    'T_Stone_N': ('rock_tile_floor/rock_tile_floor_nor_dx_2k.jpg', 'normal'),
    'T_Stone_ARM': ('rock_tile_floor/rock_tile_floor_arm_2k.jpg', 'mask'),
    'T_ScratchedMetal_D': ('blue_metal_plate/blue_metal_plate_diff_2k.jpg', 'color'),
    'T_ScratchedMetal_N': ('blue_metal_plate/blue_metal_plate_nor_dx_2k.jpg', 'normal'),
    'T_ScratchedMetal_ARM': ('blue_metal_plate/blue_metal_plate_arm_2k.jpg', 'mask'),
}


def load(path):
    return eal.load_asset(path) if eal.does_asset_exist(path) else None


def import_textures():
    tasks = []
    for name, (rel, _) in TEXTURES.items():
        source = SOURCE_DIR / rel
        if not source.is_file():
            raise RuntimeError('Missing texture: {}'.format(source))
        # Delete the texture asset to reimport a changed image.
        if eal.does_asset_exist(ASSET_DIR + '/Textures/' + name):
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', str(source))
        task.set_editor_property('destination_path', ASSET_DIR + '/Textures')
        task.set_editor_property('destination_name', name)
        task.set_editor_property('automated', True)
        task.set_editor_property('replace_existing', True)
        task.set_editor_property('save', False)
        tasks.append(task)
    if tasks:
        asset_tools.import_asset_tasks(tasks)
    textures = {}
    for name, (_, kind) in TEXTURES.items():
        tex = load(ASSET_DIR + '/Textures/' + name)
        if not tex:
            raise RuntimeError('Texture import failed: ' + name)
        if kind == 'normal':
            tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property('srgb', False)
            tex.set_editor_property('flip_green_channel', False)  # already DirectX (nor_dx)
            tex.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP)
        elif kind == 'mask':
            tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
            tex.set_editor_property('srgb', False)
            tex.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD)
        else:
            tex.set_editor_property('srgb', True)
            tex.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD)
        eal.save_loaded_asset(tex)
        textures[name] = tex
    return textures


def make_master(textures):
    name = 'M_Marau6_Surface'
    material = load(ASSET_DIR + '/' + name)
    if not material:
        material = asset_tools.create_asset(name, ASSET_DIR, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError('Material creation failed: ' + name)
    lib.delete_all_material_expressions(material)
    # WorldAlignedNormal returns a world-space normal.
    material.set_editor_property('tangent_space_normal', False)

    fdir = '/Engine/Functions/Engine_MaterialFunctions01/Texturing/'
    wat = unreal.load_object(None, fdir + 'WorldAlignedTexture.WorldAlignedTexture')
    wan = unreal.load_object(None, fdir + 'WorldAlignedNormal.WorldAlignedNormal')
    if not wat or not wan:
        raise RuntimeError('WorldAligned material functions not found')

    def expr(cls, x, y):
        return lib.create_material_expression(material, cls, x, y)

    def connect(a, out, b, inp):
        if not lib.connect_material_expressions(a, out, b, inp):
            names = []
            try:
                names = list(lib.get_material_expression_input_names(b))
            except Exception:
                pass
            raise RuntimeError('Could not connect {}.{} -> {} (inputs: {})'.format(out, inp, b.get_name(), names))

    def scalar(param, default, x, y):
        e = expr(unreal.MaterialExpressionScalarParameter, x, y)
        e.set_editor_property('parameter_name', param)
        e.set_editor_property('default_value', default)
        return e

    # TileSize (cm) -> float3 for the TextureSize pins.
    tile = scalar('TileSize', 250.0, -1500, 0)
    app2 = expr(unreal.MaterialExpressionAppendVector, -1300, 0)
    connect(tile, '', app2, 'A')
    connect(tile, '', app2, 'B')
    size3 = expr(unreal.MaterialExpressionAppendVector, -1150, 0)
    connect(app2, '', size3, 'A')
    connect(tile, '', size3, 'B')

    contrast = scalar('ProjectionContrast', 4.0, -1500, 150)

    def tex_object(param, tex, sampler, x, y):
        e = expr(unreal.MaterialExpressionTextureObjectParameter, x, y)
        e.set_editor_property('parameter_name', param)
        e.set_editor_property('texture', tex)
        e.set_editor_property('sampler_type', sampler)
        return e

    def world_aligned(func, obj, x, y):
        call = expr(unreal.MaterialExpressionMaterialFunctionCall, x, y)
        call.set_editor_property('material_function', func)
        connect(obj, '', call, 'TextureObject')
        connect(size3, '', call, 'TextureSize')
        connect(contrast, '', call, 'ProjectionTransitionContrast')
        return call

    st = unreal.MaterialSamplerType
    albedo_obj = tex_object('Albedo', textures['T_Stone_D'], st.SAMPLERTYPE_COLOR, -1000, -400)
    arm_obj = tex_object('ARM', textures['T_Stone_ARM'], st.SAMPLERTYPE_MASKS, -1000, 200)
    normal_obj = tex_object('Normal', textures['T_Stone_N'], st.SAMPLERTYPE_NORMAL, -1000, 600)

    albedo = world_aligned(wat, albedo_obj, -700, -400)
    arm = world_aligned(wat, arm_obj, -700, 200)
    normal = world_aligned(wan, normal_obj, -700, 600)

    tint = expr(unreal.MaterialExpressionVectorParameter, -500, -600)
    tint.set_editor_property('parameter_name', 'Tint')
    tint.set_editor_property('default_value', unreal.LinearColor(1, 1, 1, 1))
    base = expr(unreal.MaterialExpressionMultiply, -300, -400)
    connect(albedo, 'XYZ Texture', base, 'A')
    connect(tint, '', base, 'B')
    lib.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)

    # Poly Haven ARM: R = AO, G = roughness, B = metallic.
    mask = expr(unreal.MaterialExpressionComponentMask, -450, 100)
    mask.set_editor_property('r', True)
    mask.set_editor_property('g', False)
    mask.set_editor_property('b', False)
    connect(arm, 'XYZ Texture', mask, '')
    lib.connect_material_property(mask, '', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)

    rmask = expr(unreal.MaterialExpressionComponentMask, -450, 200)
    rmask.set_editor_property('r', False)
    rmask.set_editor_property('g', True)
    connect(arm, 'XYZ Texture', rmask, '')
    rough_mul = expr(unreal.MaterialExpressionMultiply, -250, 200)
    connect(rmask, '', rough_mul, 'A')
    connect(scalar('RoughnessScale', 1.0, -450, 260), '', rough_mul, 'B')
    lib.connect_material_property(rough_mul, '', unreal.MaterialProperty.MP_ROUGHNESS)

    mmask = expr(unreal.MaterialExpressionComponentMask, -450, 350)
    mmask.set_editor_property('r', False)
    mmask.set_editor_property('b', True)
    connect(arm, 'XYZ Texture', mmask, '')
    metal_mul = expr(unreal.MaterialExpressionMultiply, -250, 350)
    connect(mmask, '', metal_mul, 'A')
    connect(scalar('MetallicScale', 1.0, -450, 410), '', metal_mul, 'B')
    lib.connect_material_property(metal_mul, '', unreal.MaterialProperty.MP_METALLIC)

    lib.connect_material_property(normal, 'XYZ Texture', unreal.MaterialProperty.MP_NORMAL)

    lib.recompile_material(material)
    eal.save_loaded_asset(material)
    return material


def make_instance(name, parent, textures, prefix, tile_size, tint=1.0):
    mi = load(ASSET_DIR + '/' + name)
    if not mi:
        mi = asset_tools.create_asset(name, ASSET_DIR, unreal.MaterialInstanceConstant,
                                      unreal.MaterialInstanceConstantFactoryNew())
    lib.set_material_instance_parent(mi, parent)
    lib.set_material_instance_texture_parameter_value(mi, 'Albedo', textures[prefix + '_D'])
    lib.set_material_instance_texture_parameter_value(mi, 'ARM', textures[prefix + '_ARM'])
    lib.set_material_instance_texture_parameter_value(mi, 'Normal', textures[prefix + '_N'])
    lib.set_material_instance_scalar_parameter_value(mi, 'TileSize', tile_size)
    lib.set_material_instance_vector_parameter_value(mi, 'Tint', unreal.LinearColor(tint, tint, tint, 1.0))
    lib.update_material_instance(mi)
    eal.save_loaded_asset(mi)
    return mi


def is_grid(path):
    return ('/LevelPrototyping/Materials/MI_PrototypeGrid' in path
            or '/LevelPrototyping/Materials/M_PrototypeGrid' in path)


def is_orange(mat):
    if not isinstance(mat, unreal.MaterialInstance):
        return False
    try:
        c = lib.get_material_instance_vector_parameter_value(mat, 'Base Color')
    except Exception:
        return False
    return c.r > 0.4 and c.r > c.g * 1.5 and c.g > c.b


def mesh_components(actor):
    return [c for c in actor.get_components_by_class(unreal.MeshComponent)]


def process_map(map_path, stone, metal):
    unreal.EditorLoadingAndSavingUtils.load_map(map_path)
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    unreal.log_warning('ENV_MAT: {} loaded, {} actors'.format(map_path, len(actors)))
    usage = {}
    replaced = {}
    colors = {}
    for actor in actors:
        for comp in mesh_components(actor):
            n = comp.get_num_materials()
            changed = False
            for i in range(n):
                mat = comp.get_material(i)
                path = mat.get_path_name().split('.')[0] if mat else 'None'
                usage[path] = usage.get(path, 0) + 1
                if mat and path not in colors and isinstance(mat, unreal.MaterialInstance):
                    try:
                        colors[path] = lib.get_material_instance_vector_parameter_value(mat, 'Base Color')
                    except Exception:
                        pass
                if DUMP_ONLY or not mat:
                    continue
                target = None
                if is_grid(path):
                    target = stone
                elif is_orange(mat):
                    target = metal
                if target and not isinstance(actor, unreal.StaticMeshActor):
                    # Blueprint parts (doors, pads, targets) keep their own look.
                    key = 'skipped {} in {}'.format(path, actor.get_class().get_name())
                    replaced[key] = replaced.get(key, 0) + 1
                    target = None
                if target:
                    if not changed:
                        actor.modify()
                        comp.modify()
                        changed = True
                    comp.set_material(i, target)
                    key = '{} -> {}'.format(path, target.get_name())
                    replaced[key] = replaced.get(key, 0) + 1
    for path, count in sorted(usage.items()):
        unreal.log_warning('ENV_MAT: {} uses {} x{} {}'.format(
            map_path, path, count, ('color=' + str(colors[path])) if path in colors else ''))
    for key, count in sorted(replaced.items()):
        unreal.log_warning('ENV_MAT: {} replaced {} x{}'.format(map_path, key, count))
    if not DUMP_ONLY and replaced:
        if not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True):
            raise RuntimeError('Could not save ' + map_path)
        unreal.log_warning('ENV_MAT: {} saved'.format(map_path))


if DUMP_ONLY:
    stone = metal = None
else:
    textures = import_textures()
    master = make_master(textures)
    stone = make_instance('MI_Marau6_Stone', master, textures, 'T_Stone', 250.0, 0.6)  # the bright limestone washes out under the arena light
    metal = make_instance('MI_Marau6_ScratchedMetal', master, textures, 'T_ScratchedMetal', 150.0)

for map_path in MAPS:
    process_map(map_path, stone, metal)
unreal.log_warning('ENV_MAT_OK')
