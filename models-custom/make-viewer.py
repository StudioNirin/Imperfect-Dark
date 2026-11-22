#!/usr/bin/env python3
"""Build a standalone HTML viewer for one OBJ (no network, no game needed).

    python3 make-viewer.py OBJ MTL OUTPUT.html

The mesh is embedded as compact JSON. Colours are averaged from the pictures
named by the MTL, resolved relative to the OBJ's folder. Drag rotates, wheel
zooms, R resets; the buttons switch between material colours, part colours
(the OBJ's g lines) and wireframe.
"""
import json
import os
import sys

from PIL import Image


def load_obj(path):
    verts, faces, mats, parts = [], [], [], []
    faceuvs, uvs = [], []
    mat = part = None
    mnames, pnames = [], []
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            s = line.strip()
            if s.startswith("v "):
                verts.append([round(float(x), 2) for x in s.split()[1:4]])
            elif s.startswith("vt "):
                uvs.append(tuple(float(x) for x in s.split()[1:3]))
            elif s.startswith("usemtl "):
                mat = s[7:]
            elif s.startswith("g "):
                part = s[2:]
            elif s.startswith("f "):
                ids = [t.split("/") for t in s.split()[1:]]
                vi = [int(t[0]) - 1 for t in ids]
                ti = [int(t[1]) - 1 if len(t) > 1 and t[1] else -1 for t in ids]
                for k in range(1, len(ids) - 1):
                    faces.append([vi[0], vi[k], vi[k + 1]])
                    faceuvs.append([ti[0], ti[k], ti[k + 1]])
                    if mat not in mnames:
                        mnames.append(mat)
                    if part not in pnames:
                        pnames.append(part)
                    mats.append(mnames.index(mat))
                    parts.append(pnames.index(part))
    return verts, faces, mats, parts, mnames, pnames, faceuvs, uvs


def load_textures(mtlpath, objdir):
    """Material name -> picture. The dump writes PNGs the XBLA way up, so the
    OBJ's v has to be flipped when sampling (model-packs.md)."""
    pics, current = {}, None
    with open(mtlpath, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            s = line.strip()
            if s.startswith("newmtl "):
                current = s[7:]
            elif s.startswith("map_Kd ") and current:
                pic = s[7:]
                if not os.path.isabs(pic):
                    pic = os.path.normpath(os.path.join(objdir, pic))
                if os.path.exists(pic):
                    pics[current] = Image.open(pic).convert("RGB")
    return pics


def face_colors(mnames, mats, faceuvs, uvs, pics):
    """One RGB triple per face, sampled at its UVs rather than averaged over
    the whole material picture. The shotgun's red shells are one island of a
    512x512 atlas: averaging the picture painted them brown."""
    out = []
    fallback = {}
    for name in mnames:
        im = pics.get(name)
        if im is None:
            fallback[name] = (180, 170, 150)
        else:
            small = im.resize((32, 32))
            px = list(small.getdata())
            bright = [c for c in px if sum(c) > 90]
            fallback[name] = tuple(sum(c[i] for c in (bright or px)) // len(bright or px)
                                   for i in range(3))
    for mi, tis in zip(mats, faceuvs):
        im = pics.get(mnames[mi])
        if im is None or not tis or any(t < 0 or t >= len(uvs) for t in tis):
            out.append(fallback[mnames[mi]])
            continue
        w, h = im.size
        samples = []
        for t in tis:
            u, v = uvs[t]
            x = min(w - 1, max(0, int((u % 1.0) * w)))
            y = min(h - 1, max(0, int((1.0 - (v % 1.0)) * h)))
            samples.append(im.getpixel((x, y)))
        out.append(tuple(sum(s[k] for s in samples) // len(samples) for k in range(3)))
    return out


TEMPLATE = r'''<!doctype html>
<html><head><meta charset="utf-8"><title>__TITLE__ viewer</title>
<style>
 body{color:var(--foreground);font:var(--font-sans);margin:0}
 .bar{display:flex;gap:12px;align-items:center;flex-wrap:wrap;margin:0 0 8px}
 button{background:var(--card);color:var(--foreground);border:1px solid var(--border);border-radius:var(--radius);padding:4px 10px;cursor:pointer}
 canvas{width:100%;height:460px;display:block;background:#15171c;border:1px solid var(--border);border-radius:var(--radius);touch-action:none}
 .muted{color:var(--muted-foreground);font-size:.85em}
 .sw{display:inline-block;width:10px;height:10px;margin-right:4px;border:1px solid #000}
</style></head><body>
<div class="bar"><strong>__TITLE__</strong><span class="muted">drag to rotate, wheel to zoom, R to reset</span>
<button id="wm">materials</button><button id="wp">parts</button><button id="ww">wireframe</button></div>
<canvas id="c"></canvas>
<div class="muted" id="legend"></div>
<script>
__DATA__
const MAT=MATNAMES.map((_,i)=>FC[M.indexOf(i)]||[128,128,128]);
const c=document.getElementById('c'),x=c.getContext('2d');
let yaw=0.6,pitch=-0.2,zoom=1,mode='mat',drag=null;
const n=V.length, cy=[0,0,0];
for(let i=0;i<n;i++)for(let k=0;k<3;k++)cy[k]+=V[i][k]/n;
let ext=0;for(let i=0;i<n;i++)for(let k=0;k<3;k++)ext=Math.max(ext,Math.abs(V[i][k]-cy[k]));
function proj(){
 const dpr=window.devicePixelRatio||1;
 const sy=Math.sin(yaw),co=Math.cos(yaw),sp=Math.sin(pitch),cp=Math.cos(pitch),P=new Array(n);
 const s=Math.min(c.width,c.height)/dpr*0.42*zoom/ext;
 for(let i=0;i<n;i++){
  const a=V[i][0]-cy[0],b=V[i][1]-cy[1],d=V[i][2]-cy[2];
  const x1=a*co-d*sy, z1=a*sy+d*co;
  const y2=b*cp-z1*sp, z2=b*sp+z1*cp;
  const f=s/(1+z2/12000);
  P[i]=[c.width/dpr/2+x1*f,c.height/dpr/2-y2*f,z2,x1,y2,z2];
 }
 return P;
}
function shade(nx,ny,nz){return Math.max(.32,Math.min(1,.38+.62*Math.abs((nx*0.3+ny*0.5+nz*0.81))))}
function draw(){
 const dpr=window.devicePixelRatio||1,W=c.width/dpr,H=c.height/dpr;
 x.clearRect(0,0,W,H);const P=proj(),order=[];
 for(let i=0;i<F.length;i++){const f=F[i],z=(P[f[0]][2]+P[f[1]][2]+P[f[2]][2])/3;order.push([z,i])}
 order.sort((a,b)=>b[0]-a[0]);
 for(const [,i] of order){
  const f=F[i],a=P[f[0]],b=P[f[1]],d=P[f[2]];
  const ux=b[3]-a[3],uy=b[4]-a[4],uz=b[5]-a[5],vx=d[3]-a[3],vy=d[4]-a[4],vz=d[5]-a[5];
  let nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;const l=Math.hypot(nx,ny,nz)||1;
  let col;
  if(mode==='part'){col=PARTCOL[G[i]%3]}
  else{col='rgb('+FC[i].map(v=>Math.round(v*shade(nx/l,ny/l,nz/l))).join(',')+')'}
  x.beginPath();x.moveTo(a[0],a[1]);x.lineTo(b[0],b[1]);x.lineTo(d[0],d[1]);x.closePath();
  if(mode==='wire'){x.strokeStyle=col;x.lineWidth=.6;x.stroke()}
  else{x.fillStyle=col;x.fill();x.strokeStyle='rgba(0,0,0,.18)';x.lineWidth=.4;x.stroke()}
 }
}
function fit(){const r=c.getBoundingClientRect(),d=window.devicePixelRatio||1;c.width=r.width*d;c.height=r.height*d;x.setTransform(d,0,0,d,0,0);draw()}
c.addEventListener('pointerdown',e=>{drag=[e.clientX,e.clientY,yaw,pitch];c.setPointerCapture(e.pointerId)});
c.addEventListener('pointermove',e=>{if(!drag)return;yaw=drag[2]+(e.clientX-drag[0])*.009;pitch=Math.max(-1.4,Math.min(1.4,drag[3]+(e.clientY-drag[1])*.009));draw()});
c.addEventListener('pointerup',()=>drag=null);
c.addEventListener('wheel',e=>{e.preventDefault();zoom=Math.max(.2,Math.min(5,zoom*(e.deltaY>0?.9:1.1)));draw()},{passive:false});
window.addEventListener('keydown',e=>{if(e.key.toLowerCase()==='r'){yaw=.6;pitch=-.2;zoom=1;draw()}});
for(const [id,m] of [['wm','mat'],['wp','part'],['ww','wire']])
 document.getElementById(id).onclick=()=>{mode=m;draw()};
function legend(){
 let h='';
 if(mode==='part'){h=PARTNAMES.map((g,i)=>'<span><i class="sw" style="background:'+PARTCOL[i]+'"></i>'+g+'</span> ').join('')}
 else{h=MATNAMES.map((m,i)=>'<span><i class="sw" style="background:rgb('+MAT[i]+')"></i>'+m+'</span> ').join('')}
 document.getElementById('legend').innerHTML=h;
}
const _d=draw;draw=function(){_d();legend()};
new ResizeObserver(fit).observe(c);fit();
</script></body></html>'''


def main():
    if len(sys.argv) != 4:
        sys.exit("usage: make-viewer.py OBJ MTL OUTPUT.html")
    obj, mtl, out = sys.argv[1:]
    verts, faces, mats, parts, mnames, pnames, faceuvs, uvs = load_obj(obj)
    pics = load_textures(mtl, os.path.dirname(os.path.abspath(obj)))
    fc = face_colors(mnames, mats, faceuvs, uvs, pics)
    partcol = ['#4ea3ff', '#ffb454', '#7ad17a']
    data = (
        'const V=' + json.dumps(verts, separators=(',', ':')) + ';\n'
        'const F=' + json.dumps(faces, separators=(',', ':')) + ';\n'
        'const M=' + json.dumps(mats, separators=(',', ':')) + ';\n'
        'const G=' + json.dumps(parts, separators=(',', ':')) + ';\n'
        'const FC=' + json.dumps(fc, separators=(',', ':')) + ';\n'
        'const MATNAMES=' + json.dumps(mnames) + ';\n'
        'const PARTNAMES=' + json.dumps(pnames) + ';\n'
        'const PARTCOL=' + json.dumps(partcol) + ';\n'
    )
    html = (TEMPLATE.replace('__TITLE__', os.path.basename(out).replace('.html', ''))
                    .replace('__DATA__', data))
    with open(out, 'w', encoding='utf-8') as f:
        f.write(html)
    print(f'{out}: {len(html)} chars, {len(verts)} vertices, {len(faces)} faces, '
          f'{len(mnames)} materials, {len(pnames)} parts')


if __name__ == '__main__':
    main()
