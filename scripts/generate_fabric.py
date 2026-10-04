# SPDX-License-Identifier: MIT
from pathlib import Path
import numpy as np
from PIL import Image
dest=Path(__file__).resolve().parents[1]/'ArtSource/Characters'
n=1024;y,x=np.mgrid[0:n,0:n]
warp=np.sin(x*np.pi/2)*np.cos(y*np.pi/2)
fiber=.008*warp+.003*np.sin(x*.39+y*.07)
tone=np.clip(.94+.035*warp+.012*np.sin(x*.11),0,1)
Image.fromarray((np.repeat(tone[:,:,None],3,axis=2)*255).astype(np.uint8)).save(dest/'cloth_weave.png')
nx=-np.gradient(fiber,axis=1)*5;ny=-np.gradient(fiber,axis=0)*5;nz=np.sqrt(1-nx*nx-ny*ny)
Image.fromarray(((np.stack([nx,ny,nz],2)*.5+.5)*255).astype(np.uint8)).save(dest/'cloth_normal.png')
print('Authored 1024 pixel woven color and tangent-space normal surfaces.')
