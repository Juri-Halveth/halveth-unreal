# SPDX-License-Identifier: MIT
"""Bind the rendered appearance candidate, without certifying aesthetic quality."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PACKAGE = ROOT/'.local/meadow-package/Windows'
GAME = PACKAGE/'HALVETHRealms'
NAMES = ('Scarlet', 'Lucinet', 'Rachel', 'Tala', 'Orin', 'Ivo', 'Mara', 'Neri')


def require(value, message):
    if not value:
        raise ValueError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def file_record(path):
    return dict(path=path.relative_to(ROOT).as_posix(), bytes=path.stat().st_size,
                sha256=digest(path))


def png(path):
    data = path.read_bytes()
    require(data[:8] == b'\x89PNG\r\n\x1a\n', 'Not a PNG: '+path.name)
    width, height = struct.unpack('>II', data[16:24])
    return dict(file_record(path), width=width, height=height)


def log(name, predicate):
    path = ROOT/'.local'/name
    text = path.read_text(encoding='utf8', errors='replace')
    require(predicate in text, 'Missing runtime completion: '+name)
    require(not any(term in text for term in ('Assertion failed', 'Fatal error', 'TISSUE_STEP_REJECTED')),
            'Native failure in '+name)
    return path, text


build = json.loads((ROOT/'.local/meadow-build.json').read_text(encoding='utf-8-sig'))
exe = GAME/'Binaries/Win64/HALVETHRealms.exe'
require(digest(exe).upper() == build['binarySha256'].upper(), 'Stale native binary binding')
for payload in build['cookedPayload']:
    path = GAME/'Content/Paks'/payload['name']
    require(path.stat().st_size == payload['bytes'] and digest(path).upper() == payload['sha256'].upper(),
            'Cooked payload changed: '+path.name)

art = json.loads((ROOT/'ArtSource/Characters/characters.json').read_text())
require(tuple(c['name'] for c in art['characters']) == NAMES, 'Wrong character catalogue')
for c in art['characters']:
    require(c['adultAgeYears'] >= 25 and c['sourceTopology']['fullBody']
            and c['sourceTopology']['deletedCoveredFaces'] == 0, 'Adult full-body source missing')
    require(c['bodyMinimumZCm'] == 0 and c['bones'] == 163, 'Unexpected sole/rig contract')
    require(set(c['materialSlots']) == {'Skin','Hair','Sclera','Iris','Pupil'}, 'Retired clothing is active')
    require(digest(ROOT/'ArtSource/Characters'/(c['name']+'.fbx')) == c['fbxSha256'], 'FBX changed')
    require(all(m['changedVertices'] > 0 for m in c['morphGeometry'].values()), 'Neutral placeholder morph')

gallery_path, gallery = log('appearance-final-gallery.log', 'APPEARANCE_CAPTURE_COMPLETE subjects=8')
gallery_frames = re.findall(r'APPEARANCE_FRAME file=(\S+) stage=(\d+) model=(\S+) viewport=(\d+)x(\d+).*?actual_view_fov_deg=([\d.]+)', gallery)
require(len(gallery_frames) == 10, 'Incomplete fresh gallery run')
portraits = []
for index, name in enumerate(NAMES):
    frame = gallery_frames[index]
    require(frame[:3] == ('portrait-'+name+'.png', str(index), name), 'Wrong model/frame identity')
    require(frame[3:5] == ('1920','1080') and float(frame[5]) == 30., 'Wrong portrait view')
    image = png(GAME/'Preview/Appearance'/frame[0])
    require((image['width'], image['height']) == (1920,1080), 'Wrong portrait pixel dimensions')
    portraits.append(dict(name=name, image=image))

studio_path, studio = log('appearance-final-studio.log', 'APPEARANCE_CAPTURE_COMPLETE subjects=1')
pose_lines = re.findall(r'APPEARANCE_ACTUAL_POSE file=(\S+) time=([\d.-]+) yaw=([\d.-]+) pitch=([\d.-]+) blink=([\d.-]+) talk=([\d.-]+) breath=([\d.-]+) eye_left=(.*?) eye_right=(.*?) head_rotation=(.*)', studio)
require(len(pose_lines) == 4, 'Missing actual studio pose observations')
for row in pose_lines:
    require(tuple(map(float,row[1:7])) == (0.,0.,0.,0.,0.,.25), 'Studio pose/blink/gaze is not fixed')
require(len({tuple(row[7:]) for row in pose_lines}) == 1, 'Studio eye/head transforms differ')
studio_images = [png(GAME/'Preview/AppearanceStudio'/row[0]) for row in pose_lines]

motion_path, motion = log('appearance-final-motion.log', 'TISSUE_NATIVE_WITNESS result=PASS')
require('TISSUE_GPU_FIELD_WITNESS' in motion and 'result=FAIL' not in motion, 'GPU field witness failed')
bindings = re.findall(r'TISSUE_SURFACE_READY owner=(\S+).*?bound_materials=(\d+)', motion)
require(bindings and all(int(count) == 5 for owner,count in bindings), 'A facial material lacks the shared field')
audits = re.findall(r'TISSUE_SURFACE_AUDIT .*?failed=(\d+)', motion)
require(audits and set(audits) == {'0'}, 'A live tissue component failed')
rows = [line.split('\t') for line in (GAME/'QA/appearance-motion-frames.tsv').read_text().splitlines()]
times = [float(row[1]) for row in rows]
require(len(rows) >= 280 and all(b > a for a,b in zip(times,times[1:])), 'Motion coverage/timing gap')
require(all(row[2] == '0' for row in rows), 'A second camera stage entered the single-camera clip')
motion_frames = [png(GAME/'Preview/AppearanceMotion'/row[3]) for row in rows]
require(len({frame['sha256'] for frame in motion_frames}) > len(rows)*.9, 'Motion images unexpectedly repeat')
step = (times[-1]-times[0])/(len(times)-1)
native_motion = json.loads((GAME/'QA/native-appearance-motion.json').read_text())
require(abs(native_motion['observedSamplingHz']-1./step)<1e-6
        and native_motion['nominalSamplingHz']==24 and not native_motion['isFrameRateBenchmark'],
        'Intended and observed sampling are not distinguished')
video = ROOT/'.local/VISIBLE_BODIES_20s.mp4'
video_record = None
if video.is_file():
    info = json.loads(subprocess.run(['ffprobe','-v','error','-show_streams','-of','json',str(video)],
        check=True,capture_output=True,text=True).stdout)
    streams = info['streams']; visual=[s for s in streams if s['codec_type']=='video']
    require(len(visual)==1 and all(s['codec_type']!='audio' for s in streams), 'Unexpected video/audio streams')
    stream=visual[0]
    require(stream['avg_frame_rate']=='15/1' and int(stream['nb_frames'])==len(rows)
            and abs(float(stream['duration'])-20.)<.001, 'Encoded timing differs from capture')
    video_record=dict(file_record(video), codec=stream['codec_name'], width=stream['width'],height=stream['height'],
        frames=int(stream['nb_frames']), durationSeconds=float(stream['duration']),encodedFps=15,audio='NONE')
start_path,start_log=log('appearance-cached-start-probe.log','ASTRAL_NATIVE_PROBE result=PASS')
receipt = dict(schema='halveth.visible-appearance-candidate.v1',
    recordedAtUtc=datetime.now(timezone.utc).isoformat(),
    status='NATIVE_ART_CANDIDATE_NOT_HUMAN_ACCEPTED',
    priorUserReview='USER_REJECTED_VISUAL_QUALITY',
    currentUserReview='PENDING',
    visualReview=dict(reviewer='CURRENT_CODEX_AGENT',
        coverage='Eight final world portraits and five of 300 final motion stills were visually inspected',
        motionFramesInspected=[0,75,150,225,299],
        fullVideoPlayback='NOT_PERFORMED',
        finding='Visible source and meadow changes; eyes, expression, gait, repeated tree art and secondary resource labels remain open'),
    scope='Native Windows Unreal candidate: adult body/face art, shared surface field and short geometric meadow',
    packageBuild=build, executable=file_record(exe),
    normalStart=dict(entry='Start-Astralis.cmd', desktopShortcut='HALVETH Portal Garden',
        cacheProbe=file_record(start_path), scope='Actual launcher cache reused with deliberately absent engine path; native traversal probe passed'),
    catalogue=file_record(ROOT/'ArtSource/Characters/characters.json'),
    portraits=portraits,
    worldViews=[png(GAME/'Preview/Appearance'/name) for name in ('world-body.png','meadow-ground.png')],
    studio=dict(images=studio_images, poseObservations=pose_lines,
        held='same model, seed, camera, actual eye/head transforms, pose time, gaze, blink, talk, breath; skin field off',
        lighting='declared white light control, EV12; not ordinary gameplay lighting'),
    movement=dict(frameCount=len(rows), firstSimulationTime=times[0], lastSimulationTime=times[-1],
        meanSampleIntervalSeconds=step, encodedFrameRate=1./step,
        timing=file_record(GAME/'QA/appearance-motion-frames.tsv'), frames=motion_frames,
        materialBindings=bindings, video=video_record,
        requestedSamplingHz=native_motion['nominalSamplingHz'], observedSamplingHz=native_motion['observedSamplingHz'],
        isFrameRateBenchmark=False,
        scope='Fixed camera position after initial placement, with tracking orientation; native walking/shared five-material field; no dialogue/drop fixture'),
    logs=[file_record(p) for p in (gallery_path,studio_path,motion_path)],
    limits=['No photorealism or human visual acceptance certification',
        'Portraits are material/pose fixtures with the live skin field disabled; movement uses the actual five-material field',
        'GPU witness checks the shared field, not every final skinned pixel',
        'Eight authored adult source bodies with seeded variation; no cell/organ biology',
        'Fixed simulation timing and offline rendering are not a normal-play FPS benchmark'])
out = ROOT/'QA/native-visible-appearance.json'
out.write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf8', newline='\n')
print(json.dumps(dict(status=receipt['status'], portraits=len(portraits), motionFrames=len(rows),
    encodedFrameRate=1./step, binarySha256=digest(exe)), indent=2))
