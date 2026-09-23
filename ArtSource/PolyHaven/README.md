# Poly Haven PBR ground materials

Two original Poly Haven material sets, downloaded on 23 September 2026 for the HALVETH Portal Garden Unreal project. The six source images are **2048 × 2048 PNG texture maps**, totalling **37,668,548 bytes** (approximately 37.7 MB). They are unchanged provider files, with byte counts and provider MD5 checks verified. Each file also has a local SHA-256 in `MANIFEST.json`.

| Set | Creator credits | Physical tile | Suggested Unreal material |
| --- | --- | --- | --- |
| [Cobblestone Floor 03](https://polyhaven.com/a/cobblestone_floor_03) | Rob Tuytel | 240 × 240 cm | `M_PH_CobblestoneFloor03` |
| [Forest Ground 04](https://polyhaven.com/a/forest_ground_04) | Rob Tuytel; Rico Cilliers | 315 × 315 cm | `M_PH_ForestGround04` |

The cobblestone provides weathered stone paving with dirt and small plants in its joints. The forest set provides earthy ground with pebbles and larger stones. These are ground-material source maps, not screenshots or example renders.

## Source files and import settings

Each asset's subfolder contains `<asset>_diff_2k.png`, `<asset>_nor_dx_2k.png` and `<asset>_rough_2k.png`.

| Suffix | Material input | Import setting |
| --- | --- | --- |
| `_diff_2k.png` | Base Color | sRGB enabled; color texture |
| `_nor_dx_2k.png` | Normal | Normal-map compression and linear sampling; DirectX convention already supplied, so no additional green-channel inversion |
| `_rough_2k.png` | Roughness | sRGB disabled; linear mask/grayscale data; sample the red channel |

Use world-space or mesh UV scaling consistent with each tile size. The physical measurements were read from the provider API; its [official schema](https://raw.githubusercontent.com/Poly-Haven/Public-API/master/swagger.yml) specifies millimetres, converted here to Unreal centimetres. The original PNGs are 8 bits per channel. Their dimensions and file signatures were checked, and both albedos were visually inspected.

Displacement, AO, metallic maps, meshes, models and preview renders were not downloaded. These sets provide surface color, lighting normals and roughness; they do not by themselves create geometric displacement. All six texture maps fit within the delegated 150,000,000-byte download budget. The Unreal import, material connections and in-engine appearance are separate integration results.

## License and source

Poly Haven publishes these assets under **CC0-1.0**, permitting modification, redistribution and commercial use. The asset license is separate from the terms for Poly Haven's live API. These files are downloaded source assets; the game does not need a Poly Haven login, API key or network connection to render them. [Official asset license](https://polyhaven.com/license)

Creator credits above are retained for provenance. `LICENSE-CC0-1.0.txt` contains the official CC0 text. This is third-party original artwork from Poly Haven and its credited artists, not HALVETH-created artwork and not an MIT-relicensed asset.

Download URLs, official file sizes and MD5 checksums came from the actual `/files/{asset_id}` API responses, and creator and scale metadata came from `/info/{asset_id}`. `MANIFEST.json` records the chosen URLs and exact hashes. The import requests used `HALVETH-AssetPreparation/0.1 (local CC0 material import)` as the User-Agent. No account, payment, paid subscription or private source was used.

Official references checked on the download date:

- [Poly Haven API documentation](https://polyhaven.com/our-api)
- [Poly Haven API Terms](https://github.com/Poly-Haven/Public-API/blob/master/ToS.md)
- [Texture map conventions](https://docs.polyhaven.com/en/technical-standards/textures)
- [CC0 legal code](https://creativecommons.org/publicdomain/zero/1.0/legalcode)

The required attribution and recognizable application header for any future live API integration are distinct from the CC0 license on the downloaded assets. The API server's own AGPL code was not downloaded or incorporated.
