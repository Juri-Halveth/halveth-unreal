"""Bind this candidate's local native observations; not a whole-game certificate."""
from pathlib import Path
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[1]
LOCAL = ROOT / '.local'
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    names = ['tissue-accepted-positive.log', 'tissue-accepted-disconnected.log',
             'tissue-final-capture.log', 'tissue-living-probe.log']
    logs = {name: (LOCAL / name).read_text(encoding='utf-8', errors='strict') for name in names}
    positive, negative, capture, living = [logs[name] for name in names]
    for text in [positive, capture]:
        if 'TISSUE_NATIVE_WITNESS result=PASS' not in text or not re.search(r'TISSUE_GPU_FIELD_WITNESS .* result=PASS ', text):
            raise SystemExit('Positive native field witness is missing.')
    if 'TISSUE_NATIVE_WITNESS result=FAIL' not in negative or not re.search(r'TISSUE_GPU_FIELD_WITNESS .* result=FAIL ', negative):
        raise SystemExit('Disconnected field must be rejected by its GPU witness.')
    if 'readback_cm=X=0.000 Y=0.000 Z=0.000' not in negative:
        raise SystemExit('Disconnected field did not yield the expected zero readback.')
    for text in [positive, capture, living]:
        if any(marker in text for marker in ['TISSUE_STEP_REJECTED', 'Assertion failed', 'Fatal error:']):
            raise SystemExit('Accepted log contains a rejection or crash.')
    if 'LIVING_NATIVE_PROBE result=PASS' not in living:
        raise SystemExit('Native flight/population probe is missing.')
    fields = re.findall(r'TISSUE_SURFACE_READY owner=\S+ field=(\S+)', positive)
    if len(fields) < 2 or len(fields) != len(set(fields)):
        raise SystemExit('Per-body GPU field identities are not distinct.')
    sources = [p for p in (ROOT / 'Source/HALVETHRealms').rglob('*') if p.is_file() and
               ('Tissue' in p.name or p.name in ['ResidentContinuity.h', 'HALVETHLivingWorld.cpp', 'LivingWorldModel.h'])]
    package = json.loads((LOCAL / 'tissue-native-binding.json').read_text(encoding='utf-8-sig'))
    current = json.loads((LOCAL / 'meadow-build.json').read_text(encoding='utf-8-sig'))
    if package != current:
        raise SystemExit('Native package binding changed after the recorded probes.')
    binary = LOCAL / 'meadow-package/Windows/HALVETHRealms/Binaries/Win64/HALVETHRealms.exe'
    if sha(binary).upper() != current['binarySha256']:
        raise SystemExit('Native binary no longer matches the package binding.')
    folder = LOCAL / 'meadow-package/Windows/HALVETHRealms'
    timing = folder / 'QA/tissue-motion-frames.tsv'
    frames = sorted((folder / 'Preview/Tissue').glob('frame-*.png'))
    if len(frames) != 300 or len(timing.read_text(encoding='utf-8-sig').splitlines()) != 300:
        raise SystemExit('Capture requires exactly 300 bound requests and files.')
    errors = re.findall(r'TISSUE_GPU_FIELD_WITNESS .*error_cm=([0-9.]+)', positive + capture)
    means = re.findall(r'TISSUE_SURFACE_AUDIT .*mean_update_ms=([0-9.]+)', positive)
    receipt = {
        'schema': 'halveth.native-tissue-candidate.v1',
        'evidenceScope': 'Native shared shader field, source-bound continuity policy, isolated gameplay probe and capture',
        'baseCommit': '5556dd3af8e222a3512963a11d46d5dc18b1d998',
        'sourceDigest': current['sourceDigest'], 'nativeBinarySha256': current['binarySha256'],
        'builtAt': current['builtAt'], 'sources': {p.relative_to(ROOT).as_posix(): sha(p) for p in sorted(sources)},
        'logs': {name: sha(LOCAL / name) for name in names},
        'gpuField': {'positivePassed': True, 'disconnectedRejected': True, 'maxComparisonErrorCm': max(map(float, errors)),
                     'preparedBodiesAcrossRun': len(fields), 'distinctFieldIdentities': len(set(fields)),
                     'wholeSkinnedPixelReadback': False},
        'cpuCarrier': {'largestPerBodyMeanUpdateMs': max(map(float, means)), 'totalRenderCostMeasured': False},
        'capture': {'frames': len(frames), 'width': 1280, 'height': 720, 'simulationSeconds': 20,
                    'encodedFps': 15, 'cameraPosition': 'one fixed position after initial fixture placement',
                    'cameraRotation': 'tracks guide', 'gravityDropFixture': False, 'dialogueFixture': False,
                    'timingSha256': sha(timing),
                    'frameHashes': {p.name: sha(p) for p in frames}},
        'media': {name: sha(ROOT / name) for name in ['Preview/TISSUE_NATIVE.png', 'Preview/TISSUE_NATIVE.gif']},
        'localVideo': {'name': 'TISSUE_NATIVE_20s.mp4', 'sha256': sha(LOCAL / 'TISSUE_NATIVE_20s.mp4')},
        'negativeProcessExitCode': 0,
        'negativeDecisionOracle': 'Explicit GPU field mismatch and TISSUE_NATIVE_WITNESS FAIL; exit code alone is insufficient',
        'limits': ['Existing rest surfaces and skeletal gait remain.', 'No cell, protein, organ or calibrated biological simulation.',
                   'GPU readback covers the shared field operator, not all final skinned vertices or pixels.',
                   'Actor handoff policy is source/portable-tested; runtime snapshot does not cover every camera path.',
                   'CPU carrier timing is not a whole-game FPS benchmark.', 'Native acceptance is not human visual acceptance.']}
    path = ROOT / 'QA/native-tissue-candidate.json'
    path.write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'receipt': path.relative_to(ROOT).as_posix(), 'gpuField': receipt['gpuField'], 'captureFrames': len(frames)}))


if __name__ == '__main__':
    main()
