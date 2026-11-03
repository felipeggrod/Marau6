"""Reimport only the sky panorama, preserving the planet assets and level."""

from pathlib import Path
import unreal

source = Path(unreal.Paths.project_dir()) / 'Content' / 'Marau6' / 'Sky' / 'Marau6_SpaceSky_Panorama.png'
if not source.is_file():
    raise RuntimeError('Missing panorama: {}'.format(source))

task = unreal.AssetImportTask()
task.set_editor_property('filename', str(source))
task.set_editor_property('destination_path', '/Game/Marau6/Sky')
task.set_editor_property('destination_name', 'T_Marau6_SpaceSky')
task.set_editor_property('automated', True)
task.set_editor_property('replace_existing', True)
task.set_editor_property('save', True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.EditorAssetLibrary.load_asset('/Game/Marau6/Sky/T_Marau6_SpaceSky')
if not texture or texture.blueprint_get_size_x() != 8192 or texture.blueprint_get_size_y() != 4096:
    raise RuntimeError('8K sky texture import failed')
unreal.log_warning('SPACE_SKY_REIMPORTED: 8192x4096 dark panorama')
