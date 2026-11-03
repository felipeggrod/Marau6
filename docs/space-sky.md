# Space sky in the test level

`Content/FirstPerson/Lvl_FirstPerson.umap` contains a non-colliding, two-sided sky sphere named `Marau6 Space Sky`. Its collision profile must be `NoCollision` (setting only Collision Enabled does not survive a reload) and Can Ever Affect Navigation is off. If the sphere keeps collision, it fills the whole arena: the runtime navmesh ends up empty, the destruction sections cannot spawn and the round never becomes ready ("Backend nao esta pronto"). Its unlit material uses the 8192×4096 panorama in `Content/Marau6/Sky`. The dark, planet-free artwork was generated from the original composition, then resampled to 8K for the imported texture.

The ochre and blue planets are separate transparent, unlit planes inside the sky sphere. They face the arena, so their silhouettes stay round instead of stretching across the sky sphere's UVs. Their source images are 1254×1254; the projected sizes are much smaller than the 8K backdrop.

The existing daylight atmosphere and height fog actors remain in the map. `[SystemSettings]` in `Config/DefaultEngine.ini` disables both render effects so they do not cover the space panorama. These two settings currently apply to all project maps. Restart the editor to load the settings and the updated map, then use the normal two-client PIE flow in `README.md` to inspect the sky while moving around the arena.

The panorama keeps a gold-green energy band over a mostly black starfield; warm red and magenta remain only along the lower horizon. The planets were extracted from the original generated composition into transparent images, then placed separately to keep their circular projection. To update only the background, run `scripts/reimport_space_sky.py` after changing the panorama PNG.

The asset setup can be reproduced with `scripts/setup_space_sky.py` in Unreal's Python Script commandlet while the editor is closed. `scripts/verify_space_sky.py` checks the imported 8K resolution, saved actors and materials, collision, navigation, and render settings.
