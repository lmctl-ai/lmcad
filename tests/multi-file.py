"""Ordered blocks share scene state, stop on errors, and validate all CLI args first."""
import json,os,struct,subprocess,sys
from pathlib import Path
work=Path(sys.argv[1])/'multi-file'
work.mkdir()
host=os.environ['LMCAD_HOST']
compiler=os.environ['LMSCRIPT_BIN']
schema=Path('schema/lmcad.dsl.json').resolve()
mesh=work/'shared.stl'
marker=work/'later.stl'
def compile(name,body):
 source=work/(name+'.lms'); output=work/(name+'.dslb')
 source.write_text(f'p = _dsl_ schema={json.dumps(str(schema))};\n{body}\nend _dsl_\nsavedsl(p, {json.dumps(str(output))});\n')
 subprocess.run([compiler,str(source),str(output)],check=True,capture_output=True)
 return str(output)
setup=compile('setup','scene.box(1.0, 1.0, 1.0, material());')
export=compile('export',f'scene.export_stl({json.dumps(str(mesh))});')
fail=compile('fail',f'scene.render_png({json.dumps(str(work/"no-camera.png"))}, 8, 8);')
later=compile('later',f'scene.export_stl({json.dumps(str(marker))});')
def run(*args): return subprocess.run([host,'--dsl',*args],capture_output=True,text=True)
r=run(setup,export)
assert r.returncode==0 and len(r.stdout.splitlines())==2,r
assert struct.unpack_from('<I',mesh.read_bytes(),80)[0]==12
for failure in [fail,str(work/'missing.dslb')]:
 r=run(setup,export,failure,later)
 assert r.returncode==3 and not marker.exists(),r
 records=[json.loads(line) for line in r.stdout.splitlines()]
 assert len(records)==3 and [x['ok'] for x in records]==[True,True,False],r
 assert records[-1]['phase']==('run' if failure==fail else 'load'),r
 assert mesh.exists(),'completed output must survive a later failure'
for invalid in ['--schema','']:
 r=run(later,invalid)
 assert r.returncode==2 and not marker.exists() and not r.stdout,r
print('shared scene, ordered success/errors, stop on failure, and upfront CLI validation pass')
