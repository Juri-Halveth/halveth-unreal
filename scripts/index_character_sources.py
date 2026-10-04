# SPDX-License-Identifier: MIT
"""Bind regenerated character art to its exact public byte inventory."""
from pathlib import Path, PurePosixPath
import hashlib
import json
from package_source import allowed

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / 'ArtSource/Characters'
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
sources = json.loads((ART / 'wardrobe-sources.json').read_text(encoding='utf8'))
for entry in sources['sources']:
    path = (ART / entry['path']).resolve(strict=True)
    if not path.is_relative_to(ART.resolve()) or entry['license'] != 'CC0-1.0':
        raise ValueError('Unbound wardrobe source or license')
    if sha(path) != entry['sha256']:
        raise ValueError('Authored wardrobe source changed: ' + entry['path'])
files = []
for path in sorted(ART.rglob('*')):
    if path.is_file() and path.name != 'MANIFEST.json' and allowed(PurePosixPath(path.relative_to(ROOT).as_posix())):
        if path.is_symlink() or not path.resolve().is_relative_to(ART.resolve()):
            raise ValueError('Linked art entry is not an inventory source')
        files.append({'path': path.relative_to(ART).as_posix(),
                      'bytes': path.stat().st_size, 'sha256': sha(path)})
(ART / 'MANIFEST.json').write_text(json.dumps({
    'schema': 'halveth.character-public-inventory.v1',
    'license': 'CC0-1.0', 'files': files}, indent=2) + '\n', encoding='utf8', newline='\n')
print('Character public byte inventory bound:', len(files), 'files')
