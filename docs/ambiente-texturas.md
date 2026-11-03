# Texturas do ambiente (mapas de teste)

Os mapas `/Game/FirstPerson/Lvl_FirstPerson` e `/Game/Variant_Shooter/Lvl_Shooter` trocaram o
material de grade cinza do template e o laranja dos blocos por materiais PBR do projeto.

## Fontes e licenças

Texturas 2K (JPG) do [Poly Haven](https://polyhaven.com), licença **CC0** (domínio público).
Originais em `SourceArt/Environment/` (rastreados por Git LFS); créditos em
`SourceArt/Environment/CREDITS.md`.

| Uso | Asset Poly Haven | Autor |
|---|---|---|
| Pedra (chão e paredes) | [rock_tile_floor](https://polyhaven.com/a/rock_tile_floor) | Charlotte Baglioni |
| Metal riscado (blocos) | [blue_metal_plate](https://polyhaven.com/a/blue_metal_plate) | Rob Tuytel |

Mapas usados: `diff` (cor), `nor_dx` (normal no padrão DirectX do Unreal) e `arm`
(R = AO, G = rugosidade, B = metálico).

## Materiais (`/Game/Marau6/Environment`)

- `M_Marau6_Surface`: material mestre com projeção triplanar (`WorldAlignedTexture` para cor e
  ARM, `WorldAlignedNormal` para a normal, *Tangent Space Normal* desligado). Por usar UVs em
  coordenadas de mundo, cubos e rampas de qualquer escala ficam sem esticar. Parâmetros:
  `TileSize` (cm por repetição), `ProjectionContrast`, `Tint`, `RoughnessScale`, `MetallicScale`
  e as texturas `Albedo`, `ARM`, `Normal`.
- `MI_Marau6_Stone`: pedra, `TileSize` 250 cm, `Tint` 0.6 (escurece o calcário claro).
- `MI_Marau6_ScratchedMetal`: metal azul pintado e riscado, `TileSize` 150 cm.
- Texturas importadas em `/Game/Marau6/Environment/Textures` (normais `TC_NormalMap`, ARM
  `TC_Masks` sem sRGB).

## Troca nos mapas

Só os slots dos `StaticMeshActor` dos mapas são sobrescritos; os assets do template
(`LevelPrototyping`, `MI_FirstPersonColorway`) não mudam.

- `MI_PrototypeGrid_*` / `M_PrototypeGrid` -> `MI_Marau6_Stone`
- instâncias laranja (`MI_FirstPersonColorway`) -> `MI_Marau6_ScratchedMetal`

Blueprints (portas, spawner de NPC, alvos) mantêm o visual original.

## Rodar de novo

Com o Unreal Editor fechado:

```
"D:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\UnityProjects\marau6\Marau6.uproject" -run=pythonscript -script=D:\UnityProjects\marau6\scripts\setup_environment_materials.py -unattended -nullrhi -stdout
```

O script é idempotente: reconstrói o material mestre, atualiza as instâncias e salva os pacotes
de ator (World Partition, um arquivo por ator). As linhas `ENV_MAT:` do log listam os materiais
em uso e quantos slots foram trocados. Com a variável de ambiente `MARAU6_ENV_DUMP=1` ele só
lista os materiais, sem alterar nada. Para reimportar uma textura alterada, apague o asset em
`/Game/Marau6/Environment/Textures` antes.
