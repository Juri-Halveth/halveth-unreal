"""Local asset bootstrap and binding; no Epic assets are published in Git."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, os, shutil, subprocess, sys

ROOT = Path(__file__).resolve().parents[1]
ENGINE = Path(os.environ.get('GARDEN_ENGINE', 'C:/Program Files/Epic Games/UE_5.8'))
if str(ENGINE).startswith('/c/'):
    ENGINE = Path('C:/' + str(ENGINE)[3:])
GENERATED = [f'Content/Garden/Terrain_R{i}.uasset' for i in range(4)] + [
    'Content/Garden/PortalArch.uasset', 'Content/Garden/CrystalCrown.uasset']
SOURCES = ['Source/HALVETHRealms/Public/GardenField.h',
           'Source/HALVETHRealmsEditor/Private/GardenPrepareCommandlet.cpp',
           'Source/HALVETHRealmsEditor/Private/GardenCharactersCommandlet.cpp']
CHARACTERS = ['Scarlet', 'Lucinet', 'Rachel']
CHARACTER_INPUTS = [f'ArtSource/Characters/{name}.fbx' for name in CHARACTERS] + [
    'ArtSource/Characters/skin_female.png', 'ArtSource/Characters/skin_male.png',
    'ArtSource/Characters/cloth_weave.png', 'ArtSource/Characters/cloth_normal.png',
    'ArtSource/Characters/characters.json']
CHARACTER_ASSETS = [f'Content/Characters/{name}{suffix}.uasset'
    for name in CHARACTERS for suffix in ['', '_Skeleton', 'Idle']] + [
    'Content/Characters/skin_female.uasset', 'Content/Characters/skin_male.uasset',
    'Content/Characters/cloth_weave.uasset', 'Content/Characters/cloth_normal.uasset'] + [
    f'Content/Characters/{name}_{material}.uasset' for name in CHARACTERS
    for material in ['Skin','Cloth','Leather','Metal','Hair','Sclera','Iris','Pupil']]
BOUND = GENERATED + SOURCES + ['Content/Materials/M_HalvethSurface.uasset',
    'Content/VOID/M_Terrain.uasset', 'Content/VOID/M_Water.uasset'] + CHARACTER_INPUTS + CHARACTER_ASSETS
GEOMETRY_BINDINGS = GENERATED + SOURCES[:2] + ['Content/Materials/M_HalvethSurface.uasset',
    'Content/VOID/M_Terrain.uasset', 'Content/VOID/M_Water.uasset']
HUMAN_BINDINGS = CHARACTER_INPUTS + CHARACTER_ASSETS + [SOURCES[2]]
RECEIPT = ROOT / 'Saved/GARDEN-assets-bound.json'
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()

def snapshot():
    return {rel: sha(ROOT/rel) for rel in BOUND}

def current(files):
    if not RECEIPT.is_file():return False
    saved=json.loads(RECEIPT.read_text())['files']
    return all((ROOT/rel).is_file() and saved.get(rel)==sha(ROOT/rel) for rel in files)

def bootstrap():
    missing = [rel for rel in CHARACTER_INPUTS if not (ROOT/rel).is_file()]
    if missing:
        raise SystemExit('Required CC0/own character sources are missing: '+', '.join(missing))
    if not (ROOT/'Content/VOID/M_Terrain.uasset').is_file():
        options = [ROOT/'experiments/void-native', ROOT.parent/'HALVETH_VOID_UNREAL']
        if os.environ.get('GARDEN_VOID_ROOT'):
            options.insert(0, Path(os.environ['GARDEN_VOID_ROOT']))
        void = next((p.resolve() for p in options if (p/'VOID.sh').is_file()), None)
        if void is None:
            raise SystemExit('VOID asset pipeline is required. Set GARDEN_VOID_ROOT.')
        bash = os.environ.get('GARDEN_BASH', 'C:/Program Files/Git/bin/bash.exe')
        for mode in ('download', 'prepare'):
            subprocess.run([bash, '--noprofile', '--norc', str(void/'VOID.sh'), mode, '0'], check=True)
        source = void/'Content/VOID'
        for rel in ['Models','Materials','Textures','M_Fern.uasset','M_Terrain.uasset','M_Water.uasset','Water.uasset']:
            target = ROOT/'Content/VOID'/rel
            target.parent.mkdir(parents=True, exist_ok=True)
            if (source/rel).is_dir(): shutil.copytree(source/rel, target, dirs_exist_ok=True)
            else: shutil.copy2(source/rel, target)
        shutil.copy2(void/'Saved/VOID-imports.json', ROOT/'Saved/Garden-imports.json')

def archive_generated():
    # Six exact authored assets, no recursive deletion or retail data access.
    base = (ROOT/'Content/Garden').resolve()
    history = ROOT/'Saved/history'/datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    for rel in GENERATED:
        path = (ROOT/rel).resolve()
        if path.parent != base: raise SystemExit('Generated asset is outside the authored directory.')
        if path.is_file():
            history.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, history/path.name)
            path.unlink()

mode = sys.argv[1]
if mode == 'bootstrap': bootstrap()
elif mode == 'bind':
    RECEIPT.parent.mkdir(parents=True, exist_ok=True)
    RECEIPT.write_text(json.dumps({'schema':'garden.asset-binding.v1',
        'recordedAtUtc':datetime.now(timezone.utc).isoformat(), 'files':snapshot()}, indent=2)+'\n', encoding='utf-8')
elif mode == 'verify':
    if any(not (ROOT/rel).is_file() for rel in BOUND) or not RECEIPT.is_file() or json.loads(RECEIPT.read_text())['files'] != snapshot():
        raise SystemExit('Garden generated assets need preparation for this source.')
    print('GARDEN: four landscapes and three human meshes, skeletons, animations, materials and source bindings match.')
elif mode == 'geometry-current':
    if not current(GEOMETRY_BINDINGS):raise SystemExit(1)
elif mode == 'characters-current':
    if not current(HUMAN_BINDINGS):raise SystemExit(1)
elif mode == 'archive-generated': archive_generated()
elif mode == 'qa':
    receipt = json.loads((ROOT/'Saved/HALVETH-runtime-smoke.json').read_text())
    if not receipt.get('passed'): raise SystemExit('Native gameplay check failed.')
    print('GARDEN: native gameplay, terrain collision and crystal cycle passed.')
else: raise SystemExit('Expected bootstrap, bind, verify, archive-generated or qa.')
