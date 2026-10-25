# SPDX-License-Identifier: MIT
"""Bind a completed local art candidate. No visual-quality acceptance is inferred."""
from pathlib import Path
from datetime import datetime,timezone
import csv,hashlib,json,math,re,struct,subprocess

ROOT=Path(__file__).resolve().parents[1]
GAME=ROOT/'.local/meadow-package/Windows/HALVETHRealms'
NAMES=('Scarlet','Lucinet','Rachel','Tala','Orin','Ivo','Mara','Neri')
def require(ok,why):
    if not ok:raise ValueError(why)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def record(p):
    return dict(path=p.relative_to(ROOT).as_posix(),bytes=p.stat().st_size,sha256=sha(p))
def png(p):
    data=p.read_bytes();require(data[:8]==b'\x89PNG\r\n\x1a\n','Not PNG: '+p.name)
    return dict(record(p),width=struct.unpack('>I',data[16:20])[0],height=struct.unpack('>I',data[20:24])[0])
def load(p):return json.loads(p.read_text(encoding='utf-8-sig'))

build=load(ROOT/'.local/meadow-build.json')
exe=GAME/'Binaries/Win64/HALVETHRealms.exe'
require(sha(exe).upper()==build['binarySha256'],'Binary changed after build')
for p in build['cookedPayload']:
    require(sha(GAME/'Content/Paks'/p['name']).upper()==p['sha256'],'Cooked payload changed')
layers=load(ROOT/'ArtSource/Characters/skin_layers.json')
require(tuple(c['name'] for c in layers['characters'])==NAMES,'Wrong bound identities')
for c in layers['characters']:
    require(sha(ROOT/'ArtSource/Characters'/(c['name']+'.fbx'))==c['sourceFbxSha256'],'Rest geometry changed')
    require(sha(ROOT/'ArtSource/Characters'/c['mask'])==c['maskSha256'],'UV mask changed')
    require(min(c['maxima'])>80 and min(c['nonzeroPixelCountsAbove8'])>20,'Empty cosmetic zone')

def run(mode):
    path=ROOT/f'.local/skin-gaze-{mode}-latest.json';r=load(path)
    require(r['exitCode']==0 and r['binarySha256']==build['binarySha256'],mode+' runner/binary binding failed')
    log_path=ROOT/r['log'];text=log_path.read_text(encoding='utf-8-sig',errors='replace')
    require(not re.search(r'Fatal error:|Assertion failed:|Failed to compile Material',text),mode+' native failure')
    return r,log_path,text

layer_run,layer_log,layer_text=run('Layers')
require('APPEARANCE_STUDIO_ISOLATION' in layer_text and 'subjects=8' in layer_text,'Studio not isolated')
frames=re.findall(r'APPEARANCE_FRAME file=(\S+) stage=(\d+) model=(\S+) viewport=(\d+)x(\d+) component_fov_deg=([\d.]+) actual_view_fov_deg=([\d.]+) camera_world=(.*?) camera_rotation=(.*?) mesh_world=(.*?) fixture=',layer_text)
poses=re.findall(r'APPEARANCE_ACTUAL_POSE file=(\S+) time=([\d.-]+) yaw=([\d.-]+) pitch=([\d.-]+) blink=([\d.-]+) talk=([\d.-]+) breath=([\d.-]+) eye_left=(.*?) eye_right=(.*?) head_rotation=(.*)',layer_text)
parameters=re.findall(r'APPEARANCE_STUDIO_PARAMETERS stage=(\d+) skin_scatter=([\d.]+).*?subject_seed=(\d+)',layer_text)
controls=re.findall(r'SKIN_LAYER_CONTROL model=(\S+) strength=([\d.]+)',layer_text)
require(len(frames)==len(poses)==len(parameters)==len(controls)==16,'Incomplete 16-frame layer control')
pairs=[]
for i,name in enumerate(NAMES):
    a,b=frames[2*i:2*i+2];pa,pb=poses[2*i:2*i+2]
    require(a[0]==f'layers-{name}-off.png' and b[0]==f'layers-{name}-on.png','Pair identity mismatch')
    require(a[2]==b[2]==name and a[3:7]==b[3:7]==('1920','1080','30.000000','30.000000'),'View mismatch')
    require(a[7:]==b[7:] and pa[1:]==pb[1:],'Actual camera/actor/eye/head pose changed within pair')
    require(tuple(map(float,pa[1:7]))==(0.,0.,0.,0.,0.,.25),'Pose control not fixed')
    for index in (2*i,2*i+1):
        require(int(parameters[index][2])==7919*(i+1) and float(parameters[index][1])==.08,'Identity seed or SSS differs')
    require(controls[2*i]==(name,'0.000') and controls[2*i+1]==(name,'0.850'),'Wrong layer-strength control')
    images=[png(GAME/'Preview/SkinLayers'/p[0]) for p in (a,b)]
    require(images[0]['sha256']!=images[1]['sha256'],'Pair has no pixel change; visual meaning still requires review')
    pairs.append(dict(model=name,images=images,actualCamera=a[7:9],actualMesh=a[9],actualPose=pa[1:],seed=7919*(i+1),makeupStrength=[0,.85]))

gaze_run,gaze_log,gaze_text=run('Gaze')
native=load(GAME/'QA/native-gaze.json')
require(native['passed'] and native['sharedFieldPassed'] and not native['isFrameRateBenchmark'],'Native gaze witness failed')
require('GAZE_NATIVE_WITNESS result=PASS' in gaze_text and 'TISSUE_GPU_FIELD_WITNESS' in gaze_text,'Missing actual runtime witnesses')
rows=list(csv.DictReader((GAME/'QA/gaze-frames.tsv').read_text().splitlines(),delimiter='\t'))
require(len(rows)==native['frames'] and len(rows)>=150,'Gaze coverage is incomplete')
times=[float(r['simulation_seconds']) for r in rows]
require(all(b>a for a,b in zip(times,times[1:])),'Non-monotonic gaze capture')
hz=(len(times)-1)/(times[-1]-times[0]);require(abs(hz-native['observedHz'])<1e-5,'Gaze sampling claim mismatch')
for r in rows:
    require(abs(float(r['head_yaw']))<=.640001 and abs(float(r['eye_residual_yaw']))<=.340001,'Yaw limit failed')
    require(abs(float(r['head_pitch']))<=.260001 and abs(float(r['eye_residual_pitch']))<=.230001,'Pitch limit failed')
images=[png(GAME/'Preview/Gaze'/f'frame-{i:04d}.png') for i in range(len(rows))]
require(all((p['width'],p['height'])==(1280,720) for p in images),'Wrong motion dimensions')
require(len({p['sha256'] for p in images})>len(images)*.9,'Unexpected repeated motion image')
bindings=re.findall(r'TISSUE_SURFACE_READY .*?bound_materials=(\d+)',gaze_text)
require(bindings and set(bindings)=={'5'},'Adjacent facial slots lost their field binding')

video=ROOT/'.local/SKIN_GAZE_11s.mp4';video_info=None
if video.exists():
    streams=json.loads(subprocess.run(['ffprobe','-v','error','-show_streams','-of','json',str(video)],check=True,capture_output=True,text=True).stdout)['streams']
    require(len(streams)==1 and streams[0]['codec_type']=='video','Unexpected video/audio stream')
    v=streams[0];require(int(v['nb_frames'])==len(rows) and v['avg_frame_rate']=='15/1','Encoded frame count/rate mismatch')
    video_info=dict(record(video),frames=int(v['nb_frames']),durationSeconds=float(v['duration']),encodedFps=15,audio='NONE')

start_log=ROOT/'.local/skin-gaze-cached-start-probe.log'
require(start_log.exists() and 'ASTRAL_NATIVE_PROBE result=PASS' in start_log.read_text(encoding='utf-8-sig',errors='replace'),'Normal cached-start probe missing')
receipt=dict(schema='halveth.skin-layers-gaze-candidate.v1',recordedAtUtc=datetime.now(timezone.utc).isoformat(),
    status='NATIVE_SKIN_GAZE_CANDIDATE_NOT_HUMAN_ACCEPTED',baseCommit='209b01c84cb38b78a842fac910853568f3aac817',
    currentUserReview='PENDING',
    visualReview=dict(primaryStudioImages=['layers-Scarlet-off.png','layers-Scarlet-on.png','layers-Tala-on.png','layers-Mara-on.png'],
        primaryFinalGazeFrames=[16,17,18,21,115,140],
        secondReviewReported='All eight final ON studio images plus Scarlet OFF; final gaze stills 20,44,121,159; same source/evidence cluster',
        fullVideoPlayback='NOT_PERFORMED',
        finding='Local cosmetic effect and orientation/occlusion/recovery visible; model-like anatomy, eyes and environment remain art limitations'),
    packageBuild=build,executable=record(exe),maskSources=record(ROOT/'ArtSource/Characters/skin_layers.json'),
    materialAB=dict(pairs=pairs,log=record(layer_log),runner=record(ROOT/'.local/skin-gaze-Layers-latest.json'),
        scope='Same actual camera, mesh, eye/head pose and seed within each pair; isolated EV12 studio; live skin field off'),
    gaze=dict(native=native,observedSamplingHz=hz,frames=images,timing=record(GAME/'QA/gaze-frames.tsv'),log=record(gaze_log),
        runner=record(ROOT/'.local/skin-gaze-Gaze-latest.json'),video=video_info,fieldMaterialCount=5,
        scope='Declared moving player camera, stationary live guide, real interaction and visible blocking wall; five-material field active'),
    normalStart=dict(entry='Start-Astralis.cmd',probe=record(start_log)),
    limits=['No biological skin/sensory simulation claim','No human visual-quality acceptance inferred',
        'Controller state and last evaluated bone quaternions are different observations; not a synchronous GPU trace',
        'GPU probe checks the shared deformation field, not every final skinned pixel',
        'Fixed simulation/offscreen screenshots and encoded FPS are not normal-play performance measurements'])
path=ROOT/'QA/native-skin-layers-gaze.json';path.write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(status=receipt['status'],pairs=len(pairs),gazeFrames=len(rows),observedHz=hz,binarySha256=sha(exe)),indent=2))
