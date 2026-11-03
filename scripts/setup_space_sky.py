"""Import the generated panorama and install it in the FirstPerson test map.

Run with UnrealEditor-Cmd -run=pythonscript -script=<absolute path to this file>.
"""

from pathlib import Path
import unreal


PROJECT = Path(unreal.Paths.project_dir())
SOURCE_DIR = PROJECT / 'Content' / 'Marau6' / 'Sky'
ASSET_DIR = '/Game/Marau6/Sky'
MAP = '/Game/FirstPerson/Lvl_FirstPerson'

SOURCES = {
    'T_Marau6_SpaceSky': SOURCE_DIR / 'Marau6_SpaceSky_Panorama.png',
    'T_Marau6_OchrePlanet': SOURCE_DIR / 'Marau6_OchrePlanet.png',
    'T_Marau6_BluePlanet': SOURCE_DIR / 'Marau6_BluePlanet.png',
}
for source in SOURCES.values():
    if not source.is_file():
        raise RuntimeError('Missing sky image: {}'.format(source))

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
tasks = []
for name, source in SOURCES.items():
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(source))
    task.set_editor_property('destination_path', ASSET_DIR)
    task.set_editor_property('destination_name', name)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    tasks.append(task)
asset_tools.import_asset_tasks(tasks)
textures = {name: unreal.EditorAssetLibrary.load_asset(ASSET_DIR + '/' + name)
            for name in SOURCES}
if not all(textures.values()):
    raise RuntimeError('A sky texture import failed')

def make_material(name, texture, transparent=False):
    path = ASSET_DIR + '/' + name
    material = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not material:
        material = asset_tools.create_asset(name, ASSET_DIR,
                                            unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError('Material creation failed: ' + name)
    material.set_editor_property('two_sided', True)
    material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    if transparent:
        material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    else:
        material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
        material.set_editor_property('is_sky', True)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, -350, 0)
    sample.set_editor_property('texture', texture)
    unreal.MaterialEditingLibrary.connect_material_property(
        sample, 'RGB', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if transparent:
        unreal.MaterialEditingLibrary.connect_material_property(
            sample, 'A', unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material

sky_material = make_material('M_Marau6_SpaceSky', textures['T_Marau6_SpaceSky'])
ochre_material = make_material('M_Marau6_OchrePlanet', textures['T_Marau6_OchrePlanet'], True)
blue_material = make_material('M_Marau6_BluePlanet', textures['T_Marau6_BluePlanet'], True)

unreal.EditorLoadingAndSavingUtils.load_map(MAP)
actors = unreal.EditorLevelLibrary.get_all_level_actors()
sky = next((a for a in actors if a.get_actor_label() == 'Marau6 Space Sky'), None)
if not sky:
    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
    sky.set_actor_label('Marau6 Space Sky')

component = sky.static_mesh_component
# Each actor lives in its own World Partition package; mark it dirty so it is saved.
sky.modify()
component.modify()
component.set_static_mesh(unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Sphere'))
component.set_material(0, sky_material)
component.set_editor_property('cast_shadow', False)
# A colliding sphere this size fills the arena: the navmesh ends up empty and the
# destruction sections never spawn. The profile is what survives a reload.
component.set_collision_profile_name('NoCollision')
component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
component.set_editor_property('can_ever_affect_navigation', False)
sky.set_actor_location(unreal.Vector(0, 0, 0), False, False)
sky.set_actor_scale3d(unreal.Vector(1500, 1500, 1500))

plane = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Plane')
def place_planet(label, material, position, scale):
    actor = next((a for a in actors if a.get_actor_label() == label), None)
    if not actor:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.StaticMeshActor, position)
        actor.set_actor_label(label)
    actor.modify()
    mesh = actor.static_mesh_component
    mesh.modify()
    mesh.set_static_mesh(plane)
    mesh.set_material(0, material)
    mesh.set_editor_property('cast_shadow', False)
    mesh.set_collision_profile_name('NoCollision')
    mesh.set_editor_property('can_ever_affect_navigation', False)
    actor.set_actor_location(position, False, False)
    actor.set_actor_rotation(unreal.MathLibrary.make_rot_from_z(
        unreal.Vector(-position.x, -position.y, -position.z)), False)
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    return actor

ochre = place_planet('Marau6 Ochre Planet', ochre_material,
                     unreal.Vector(35000, 12000, 20000), 170)
blue = place_planet('Marau6 Blue Planet', blue_material,
                    unreal.Vector(35000, -18000, 10500), 65)

if not unreal.EditorLevelLibrary.save_current_level():
    raise RuntimeError('Could not save FirstPerson test map')
if not unreal.EditorLoadingAndSavingUtils.save_packages(
        [ochre.get_outermost(), blue.get_outermost()], False):
    raise RuntimeError('Could not save planet actor packages')
# save_current_level skips the external actor packages that hold these actors.
if not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False):
    raise RuntimeError('Could not save the sky actor packages')
unreal.log_warning('SPACE_SKY_OK: 8K panorama and two planet planes saved')
