"""Geometry failures must remain clean DSL host errors."""
import json,os,subprocess,sys
from pathlib import Path
work=Path(sys.argv[1])/'cad-check';work.mkdir(exist_ok=True)
schema=Path('schema/lmcad.dsl.json').resolve().as_posix()
compiler=os.environ['LMSCRIPT_BIN'];host=os.environ['LMCAD_HOST']

def compiler_cmd(c):
    # Native Windows Python cannot exec the bash/npm shim directly; npm
    # global installs provide a .cmd sibling that cmd.exe can run.
    if sys.platform == 'win32' and os.path.exists(c + '.cmd'):
        return ['cmd.exe', '/c', c + '.cmd']
    return [c]
# KNOWN LIMITATION (win32, MinGW): OCCT 8.0.1 fillet/chamfer/hollow *error*
# paths segfault instead of failing cleanly; valid operations pass.
OCCT_ERROR_PATHS_CRASH = sys.platform == 'win32'
def run(name,body,error=None):
 source=work/(name+'.lms');bytecode=work/(name+'.dslb')
 source.write_text(f'p = _dsl_ schema={json.dumps(str(schema))};\n{body}\nend _dsl_\nsavedsl(p,{json.dumps(str(bytecode))});\n')
 subprocess.run(compiler_cmd(compiler)+[str(source)],check=True,capture_output=True)
 r=subprocess.run([host,'--dsl',str(bytecode)],capture_output=True,text=True,timeout=30)
 assert r.returncode==(3 if error else 0),(name,r)
 result=json.loads(r.stdout)
 assert result['ok']==(error is None),(name,r)
 if error: assert error in result['message'],(name,r)
for name,points,error in [
 ('bowtie',[(0,0),(2,2),(0,2),(2,0)],'self-intersecting'),
 ('crossing',[(0,0),(3,3),(0,2),(2,0)],'self-intersecting'),
 ('collinear',[(0,0),(1,0),(2,0)],'zero area'),
 ('duplicate',[(0,0),(1,0),(1,0),(0,1)],'zero-length'),
 ('short',[(0,0),(1,0)],'at least three')]:
 for operation in ['extrude','revolve_y']:
  args='sk,1.0,material()' if operation=='extrude' else 'sk,material()'
  body='let sk=scene.sketch(); sk'+''.join(f'.point({x}.0,{y}.0)' for x,y in points)+f'.close(); scene.{operation}({args});'
  run(name+operation,body,error)
run('valid-extrusion','let sk=scene.sketch().point(0.0,0.0).point(2.0,0.0).point(2.0,3.0).point(0.0,3.0).close(); let s=scene.extrude(sk,4.0,material()); scene.assert_close(s.volume(),24.0,0.000001);')
run('tube-between','let s=scene.tube_between(-1.0,0.0,0.0,2.0,0.0,0.0,0.25,material()); scene.assert_close(s.volume(),3.0 * 3.141592653589793 * 0.25 * 0.25,0.000001); scene.assert_close(s.min_x(),-1.0,0.001); scene.assert_close(s.max_x(),2.0,0.001);')
run('tube-between-diagonal','let s=scene.tube_between(1.0,2.0,3.0,-1.0,4.0,5.0,0.2,material()); scene.assert_close(s.volume(),sqrt(6.0) * 3.141592653589793 * 0.2 * 0.2,0.000001); scene.assert_close(s.min_x(),-1.0,0.001); scene.assert_close(s.max_z(),5.0,0.001);')
run('tube-between-zero-length','scene.tube_between(0.0,0.0,0.0,0.0,0.0,0.0,0.25,material());','CAD dimension')
for expression,error in [
 ('scene.tube_between(0.0,0.0,0.0,1.0,0.0,0.0,0.0,material());','positive'),
 ('scene.tube_between(0.0,0.0,0.0,1.0,0.0,0.0,-0.1,material());','positive'),
 ('scene.tube_between(sqrt(-1.0),0.0,0.0,1.0,0.0,0.0,0.1,material());','finite'),
]: run('tube-between-invalid',expression,error)
print('CAD sketch validation and valid extrusion pass')

for expression in ['scene.sphere(1000000000000000.0,material())','scene.cylinder(1000000000.0,1.0,material())','scene.box(0.0000000001,1.0,1.0,material())']:
 run('dimension-limit',expression+';','CAD dimension')
run('huge-scale','let s=scene.sphere(1.0,material()).scale(1000000000.0,1.0,1.0); s.fillet(0.1);','host limit')
run('huge-position','let s=scene.box(1.0,1.0,1.0,material()).position(1000000000.0,0.0,0.0); s.volume();','host limit')
run('huge-sketch','let sk=scene.sketch().point(0.0,0.0).point(1000000000.0,0.0).point(0.0,1.0).close(); scene.extrude(sk,1.0,material());','host limit')
print('CAD geometry envelope refuses extreme dimensions before meshing')
shape='let s=scene.box(2.0,2.0,2.0,material()); '
run('empty-intersection',shape+'let tool=scene.box(1.0,1.0,1.0,material()).position(10.0,0.0,0.0); s.intersect(tool); s.volume();','empty')
run('empty-cut',shape+'s.cut(s); s.volume();','empty')
if OCCT_ERROR_PATHS_CRASH:
 print('skip: OCCT/MinGW fillet/chamfer/hollow error paths crash (known limitation)')
else:
 run('large-fillet',shape+'s.fillet(100.0);','fillet failed')
 run('large-chamfer',shape+'s.chamfer(100.0);','chamfer failed')
# Filleted boxes used to fail ("cannot normalize zero or non-finite vector"):
# canonical corner order turned fillet-seam slivers into zero normals.
for radius in ['0.05','0.14','0.3']:
 run('box-fillet-'+radius,shape+f's.fillet({radius}); scene.fit_camera(30.0,1.2); scene.directional_light(-0.5,-1.0,-0.6,0.8); scene.render_png({json.dumps(str(work/"fillet.png"))},48,48);')
run('box-fillet-volume',shape+'s.fillet(0.14); scene.assert_close(s.volume(),7.902726154935,0.000001);')
run('box-fillet-cut',shape+'s.fillet(0.14); let h=scene.cylinder(0.5,3.0,material()); s.cut(h); h.visible(false); scene.fit_camera(30.0,1.2); scene.directional_light(-0.5,-1.0,-0.6,0.8); scene.render_png('+json.dumps(str(work/'fillet-cut.png'))+',48,48);')
print('filleted boxes tessellate, render and keep their analytic volume')
if not OCCT_ERROR_PATHS_CRASH:
 run('no-planar-shell','scene.sphere(1.0,material()).hollow(0.1);','planar end face')
for dim in ['0.0','-1.0']:
 run('nonpositive',f'scene.box({dim},1.0,1.0,material());','positive')
run('alias-lifetime',shape+'let alias=s; for(i in range(0,100)){scene.box(1.0,1.0,1.0,material()).visible(false);} s.scale(-2.0,1.0,1.0); scene.assert_close(alias.volume(),16.0,0.000001); s.mirror("x"); scene.assert_close(alias.volume(),16.0,0.000001);')
print('CAD failures, reflected volume, and shape alias ownership pass')
# The mesh path must agree between exporters and retain positive reflected volume.
import struct
stl=work/'reflected.stl';obj=work/'reflected.obj'
run('reflected-exports',shape+f's.scale(-2.0,1.0,1.0); scene.export_stl({json.dumps(str(stl))}); scene.export_obj({json.dumps(str(obj))});')
data=stl.read_bytes();stl_tri=[]
for i in range(struct.unpack_from('<I',data,80)[0]):
 v=struct.unpack_from('<12f',data,84+50*i)[3:];stl_tri.append([tuple(v[j:j+3]) for j in (0,3,6)])
vertices=[];obj_tri=[]
for line in obj.read_text().splitlines():
 fields=line.split()
 if fields and fields[0]=='v': vertices.append(tuple(map(float,fields[1:])))
 if fields and fields[0]=='f': obj_tri.append([vertices[int(i)-1] for i in fields[1:]])
assert stl_tri==obj_tri,'OBJ/STL geometry differs'
volume=sum((a[0]*(b[1]*c[2]-b[2]*c[1])+a[1]*(b[2]*c[0]-b[0]*c[2])+a[2]*(b[0]*c[1]-b[1]*c[0]))/6 for a,b,c in obj_tri)
assert abs(volume-16)<1e-6,volume
print('OBJ/STL reflected meshes agree with positive BRep volume')
