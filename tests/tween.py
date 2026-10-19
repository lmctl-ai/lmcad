"""Check a three-keyframe animation against independently rendered endpoints."""
import os
import json
from pathlib import Path
import subprocess
import sys

work = Path(sys.argv[1])
compiler = os.environ['LMSCRIPT_BIN']
host = os.environ['LMCAD_HOST']
schema = str(Path('schema/lmcad.dsl.json').resolve())
csv = work / 'three.csv'
csv.write_text(Path('examples/data/keyframes.csv').read_text() + '2.0,2.0,1.0,0.0,0.2,0.9,0.1\n')
source = Path('examples/tween.lms').read_text().replace('../schema/lmcad.dsl.json', schema).replace('examples/data/keyframes.csv', str(csv)).replace('build/tween-', str(work / 'frame-')).replace('build/tween.apng', str(work / 'multi.apng'))

def run(name, text):
    path = work / (name + '.lms'); output = work / (name + '.dslb')
    old_save = 'savedsl(p, "build/tween.dslb");'
    if old_save in text:
        text = text.replace(old_save, f'savedsl(p, {json.dumps(str(output))});')
    elif 'savedsl(' not in text:
        text += f'\nsavedsl(p, {json.dumps(str(output))});\n'
    path.write_text(text)
    subprocess.run([compiler, str(path), str(output)], check=True, stdout=subprocess.DEVNULL)
    subprocess.run([host, '--dsl', str(output)], check=True, stdout=subprocess.DEVNULL)

run('multi', source)
frames = [work / f'frame-{i}.png' for i in range(59)]
assert len(list(work.glob('frame-*.png'))) == 59
assert len({p.read_bytes() for p in frames}) == 59, 'duplicated boundary frames'
reference = source.split('let frames_per_segment')[0] + f'''
for (i in range(0, keys.len())) {{
  let key = keys.at(i);
  shape.position(key.x(), key.y(), key.z()).color(key.r(), key.g(), key.b());
  scene.render_png("{work}/ref-" + str(i) + ".png", 160, 120);
}}
end _dsl_
'''
run('endpoints', reference)
for i, frame in enumerate([0, 29, 58]):
    assert frames[frame].read_bytes() == (work / f'ref-{i}.png').read_bytes(), (i, frame)
print('59 unique frames; first, shared boundary, and t=1 endpoint exact')
