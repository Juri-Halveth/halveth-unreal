"""MIT. Deterministic seamless mineral microrelief, normal and roughness maps."""
from pathlib import Path
import hashlib
import json
import numpy as np
from PIL import Image

def generate(root: Path, size=1024, seed=2301026):
    root.mkdir(parents=True,exist_ok=True)
    rng=np.random.default_rng(seed)
    y,x=np.mgrid[:size,:size]/size
    height=np.zeros((size,size),np.float64)
    for frequency,amplitude in [(2,.32),(5,.20),(13,.11),(31,.055),(75,.018),(183,.006)]:
        for _ in range(7):
            kx=int(rng.integers(-frequency,frequency+1));ky=int(rng.integers(-frequency,frequency+1))
            height+=amplitude*np.sin(2*np.pi*(kx*x+ky*y)+rng.uniform(0,2*np.pi))/7
    height+=rng.normal(0,.002,(size,size))
    dx=(np.roll(height,-1,axis=1)-np.roll(height,1,axis=1))*size*.025
    dy=(np.roll(height,-1,axis=0)-np.roll(height,1,axis=0))*size*.025
    normal=np.stack([-dx,dy,np.ones_like(dx)],axis=-1)
    normal/=np.linalg.norm(normal,axis=-1,keepdims=True)
    Image.fromarray(np.uint8(np.clip(normal*.5+.5,0,1)*255)).save(root/'mineral_normal.png')
    rough=np.clip(.80+height*.4+rng.normal(0,.018,height.shape),.45,.98)
    Image.fromarray(np.uint8(rough*255)).save(root/'mineral_roughness.png')
    files={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(root.glob('*.png'))}
    (root/'SOURCE.json').write_text(json.dumps(dict(author='Juri Janovski',license='MIT',method='Periodic integer-frequency Fourier field and wrapped central differences; synthetic mineral appearance.',generatorSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),seed=seed,size=size,files=files),indent=2)+'\n',encoding='utf8')
    print('Generated deterministic 1024px tileable mineral maps; no reference pixels copied.')

if __name__=='__main__':generate(Path(__file__).resolve().parents[1]/'ArtSource/Astral')
