"""Exercise renderer settings through the public DSL and compare actual output."""
import json,os,subprocess,sys
from pathlib import Path
work=Path(sys.argv[1])/'render-settings';work.mkdir(exist_ok=True)
compiler=os.environ['LMSCRIPT_BIN'];host=os.environ['LMCAD_HOST']

def compiler_cmd(c):
    # Native Windows Python cannot exec the bash/npm shim directly; npm
    # global installs provide a .cmd sibling that cmd.exe can run.
    if sys.platform == 'win32' and os.path.exists(c + '.cmd'):
        return ['cmd.exe', '/c', c + '.cmd']
    return [c]
schema=Path('schema/lmcad.dsl.json').resolve().as_posix()
def run(name,body,error=None):
 source=work/(name+'.lms');bytecode=work/(name+'.dslb')
 source.write_text(f'p = _dsl_ schema={json.dumps(str(schema))};\n{body}\nend _dsl_\nsavedsl(p,{json.dumps(str(bytecode))});\n')
 subprocess.run(compiler_cmd(compiler)+[str(source)],check=True,capture_output=True)
 r=subprocess.run([host,'--dsl',str(bytecode)],capture_output=True,text=True,timeout=30)
 assert r.returncode==(3 if error else 0),(name,r)
 if error: assert error in json.loads(r.stdout)['message'],(name,r)
setup='scene.box(1.0,1.0,1.0,material()); scene.camera(45.0,3.0,2.0,5.0,0.0,0.0,0.0); scene.ambient(0.0);'
def render(name,settings):
 path=work/(name+'.png')
 run(name,setup+settings+f'scene.render_png({json.dumps(str(path))},31,23);')
 return path.read_bytes()
for which in ['fill','key']:
 fixed='scene.directional_light(-1.0,-1.0,-1.0,0.0);' if which=='fill' else 'scene.fill_light(-1.0,-1.0,-1.0,0.0);'
 method='fill_light' if which=='fill' else 'directional_light'
 via_handle=render(which+'-handle',fixed+f'let l=scene.{method}(-1.0,-1.0,-1.0,0.0); l.intensity(0.8);')
 direct=render(which+'-direct',fixed+f'scene.{method}(-1.0,-1.0,-1.0,0.8);')
 assert via_handle==direct,which+' handle changes the wrong light'
for method in ['fill_light','directional_light']:
 for args,error in [('0.0,0.0,0.0,1.0','nonzero'),('1.0,0.0,0.0,-1.0','negative'),('sqrt(-1.0),0.0,0.0,1.0','finite')]:
  run('invalid-light',f'scene.{method}({args});',error)
for ambient in ['-0.1','sqrt(-1.0)']:
 run('invalid-ambient',f'scene.ambient({ambient});','ambient level')
print('independent light handles and finite bounded lighting settings pass')

run('empty-fit','scene.fit_camera(45.0,1.0);','at least one visible')
for fov,margin in [('0.0','1.0'),('180.0','1.0'),('45.0','0.5'),('sqrt(-1.0)','1.0')]:
 run('invalid-fit',f'scene.box(1.0,1.0,1.0,material());scene.fit_camera({fov},{margin});','camera')
# A fitted camera must adapt to a portrait render, keeping a margin at every edge.
import struct,zlib
portrait=work/'portrait.png'
run('portrait-fit',f'scene.background(0.0,0.0,0.0);scene.box(2.0,2.0,2.0,material());scene.fit_camera(45.0,1.1);scene.render_png({json.dumps(str(portrait))},31,127);')
b=portrait.read_bytes();pos=8;idat=b''
while pos<len(b):
 n=struct.unpack_from('>I',b,pos)[0]
 if b[pos+4:pos+8]==b'IDAT':idat+=b[pos+8:pos+8+n]
 pos+=12+n
raw=zlib.decompress(idat);rows=[raw[y*94+1:(y+1)*94] for y in range(127)]
assert any(any(row) for row in rows),'fit rendered nothing'
assert not any(rows[0]+rows[-1]) and all(not any(row[:3]+row[-3:]) for row in rows),'portrait fit clips geometry'
print('camera fit handles portrait aspect, margin, and invalid inputs')

assert render('ambient-clamped','scene.ambient(2.0);')==render('ambient-one','scene.ambient(1.0);'),'ambient above one must clamp to full illumination'

tiny=work/'tiny-fit.ppm'
run('tiny-fit',f'scene.background(0.0,0.0,0.0);scene.box(0.001,0.001,0.001,material());scene.fit_camera(45.0,1.1);scene.render_ppm({json.dumps(str(tiny))},31,31);')
pixels=tiny.read_bytes().split(b'\n',3)[3]
assert sum(x!=0 for x in pixels)>300,'tiny fitted geometry is invisible or subpixel-sized'
print('tiny geometry retains usable fitted framing')
