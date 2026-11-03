import unreal

unreal.EditorLoadingAndSavingUtils.load_map('/Game/FirstPerson/Lvl_FirstPerson')
actors = unreal.EditorLevelLibrary.get_all_level_actors()
sky = next((a for a in actors if a.get_actor_label() == 'Marau6 Space Sky'), None)
assert sky is not None, 'Sky actor missing from saved map'
component = sky.static_mesh_component
assert component.get_material(0).get_path_name().startswith('/Game/Marau6/Sky/M_Marau6_SpaceSky.')
assert component.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION
assert str(component.get_collision_profile_name()) == 'NoCollision'
assert not component.get_editor_property('can_ever_affect_navigation'), 'Sky must not affect navigation'
assert len([a for a in actors if a.get_actor_label() == 'Marau6 Space Sky']) == 1
texture = unreal.EditorAssetLibrary.load_asset('/Game/Marau6/Sky/T_Marau6_SpaceSky')
assert texture.blueprint_get_size_x() >= 8192 and texture.blueprint_get_size_y() >= 4096
for label, material in (
        ('Marau6 Ochre Planet', 'M_Marau6_OchrePlanet'),
        ('Marau6 Blue Planet', 'M_Marau6_BluePlanet')):
    planet = next((a for a in actors if a.get_actor_label() == label), None)
    assert planet is not None, '{} missing'.format(label)
    plane = planet.static_mesh_component
    assert plane.get_material(0).get_path_name().startswith('/Game/Marau6/Sky/' + material + '.')
    assert plane.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION
    assert str(plane.get_collision_profile_name()) == 'NoCollision'
    assert not plane.get_editor_property('can_ever_affect_navigation')
    assert plane.get_editor_property('static_mesh').get_path_name().startswith('/Engine/BasicShapes/Plane.')
assert unreal.SystemLibrary.get_console_variable_int_value('r.Fog') == 0
assert unreal.SystemLibrary.get_console_variable_int_value('r.SkyAtmosphere') == 0
unreal.log_warning('SPACE_SKY_VERIFIED: 8K texture, two planet planes, collision, navigation and render settings')
