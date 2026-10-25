# SPDX-License-Identifier: MIT
"""Finite, local-space art masks. No measured tissue or biological claim."""
import math


def bump(point, center, radii):
    if any(not math.isfinite(v) for v in (*point, *center, *radii)) or min(radii) <= 0:
        raise ValueError('Finite local-space coordinates and positive radii required')
    q = sum(((p-c)/r)**2 for p,c,r in zip(point,center,radii))
    return max(0., 1.-q)**2


def landmarks(bones):
    """Exact exported rest bones; centimetres in the authored FBX local frame."""
    left,right=bones['eye.L'],bones['eye.R']
    upper,lower=bones['oris06'],bones['oris05']
    corner_l,corner_r=bones['oris03.L'],bones['oris03.R']
    span=abs(left[0]-right[0]); scale=span/6.
    if not 3.5 < span < 9.:
        raise ValueError('Unexpected centimetre-space eye separation')
    mouth=tuple((a+b)*.5 for a,b in zip(upper,lower))
    half_width=max(abs(corner_l[0]-mouth[0]),abs(corner_r[0]-mouth[0]))*1.55
    half_height=max(.48*scale,abs(upper[2]-lower[2])*1.6)
    return dict(eyes=[left,right],mouth=mouth,lipRadii=[half_width,1.8*scale,half_height],
                scale=scale,eyeMid=tuple((a+b)*.5 for a,b in zip(left,right)))


def zones(point, marks):
    s=marks['scale']; x,y,z=point; eyes=marks['eyes']; mid=marks['eyeMid']
    # Compact support excludes the entire torso, back of head and neck.
    if z < mid[2]-11*s or z > mid[2]+8*s or y > mid[1]+1.5*s:
        return (0.,0.,0.,0.)
    lip=bump(point,marks['mouth'],marks['lipRadii'])
    cheeks=eyelids=0.
    for eye in eyes:
        cheek=(mid[0]+(eye[0]-mid[0])*1.24,eye[1]-.8*s,eye[2]-3.15*s)
        cheeks=max(cheeks,bump(point,cheek,(2.3*s,3.8*s,1.95*s)))
        # Upper lid and outer fold; only skin polygons are rasterised.
        lid=(eye[0],eye[1]-1.5*s,eye[2]+.64*s)
        eyelids=max(eyelids,bump(point,lid,(1.9*s,2.2*s,.92*s)))
    nose=(mid[0],mid[1]-3.1*s,mid[2]-1.5*s)
    forehead=(mid[0],mid[1]+.05*s,mid[2]+3.5*s)
    chin=(marks['mouth'][0],marks['mouth'][1]+.9*s,marks['mouth'][2]-2.2*s)
    oil=max(bump(point,nose,(1.35*s,2.7*s,2.1*s)),
            bump(point,forehead,(3.1*s,3.1*s,2.9*s)),
            bump(point,chin,(1.7*s,2.5*s,1.4*s)))
    return tuple(min(1.,max(0.,v)) for v in (lip,cheeks,eyelids,oil))
