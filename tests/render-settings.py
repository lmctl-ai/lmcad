"""Exercise renderer settings through the public DSL and compare actual output."""
import json,os,subprocess,sys
from pathlib import Path
work=Path(sys.argv[1])/'render-settings';work.mkdir(exist_ok=True)
compiler=os.environ['LMSCRIPT_BIN'];host=os.environ['LMCAD_HOST']
schema=Path('schema/lmcad.dsl.json').resolve()
def run(name,body,error=None):
 source=work/(name+'.lms');bytecode=work/(name+'.dslb')
 source.write_text(f'p = _dsl_ schema={json.dumps(str(schema))};\n{body}\nend _dsl_\nsavedsl(p,{json.dumps(str(bytecode))});\n')
 subprocess.run([compiler,str(source)],check=True,capture_output=True)
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
for ambient in ['-0.1','1.1','sqrt(-1.0)']:
 run('invalid-ambient',f'scene.ambient({ambient});','ambient level')
print('independent light handles and finite bounded lighting settings pass')
