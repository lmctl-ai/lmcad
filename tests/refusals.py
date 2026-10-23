"""Exercise validation through compiled DSL, rather than only native helpers."""
import os
from decimal import Decimal
from pathlib import Path
import subprocess
import sys

root = Path.cwd().as_posix()
work = Path(sys.argv[1])
compiler = os.environ.get('LMSCRIPT_BIN', 'lmscript')

def compiler_cmd(c):
    # Native Windows Python cannot exec the bash/npm shim directly; npm
    # global installs provide a .cmd sibling that cmd.exe can run.
    if sys.platform == 'win32' and os.path.exists(c + '.cmd'):
        return ['cmd.exe', '/c', c + '.cmd']
    return [c]
host = os.environ.get('LMCAD_HOST', 'build/lmcad')

def refuses(name, body, error):
    source = work / (name + '.lms')
    bytecode = work / (name + '.dslb')
    source.write_text(f'p = _dsl_ schema="{root}/schema/lmcad.dsl.json";\n{body}\nend _dsl_\nsavedsl(p, "{bytecode.as_posix()}");\n')
    subprocess.run(compiler_cmd(compiler)+[ str(source)], check=True, stdout=subprocess.DEVNULL)
    run = subprocess.run([host, '--dsl', str(bytecode)], capture_output=True, text=True)
    assert run.returncode == 3 and error in run.stdout, (name, run.returncode, run.stdout, run.stderr)

shape = 'let mat = material(); let s = scene.box(2.0, 2.0, 2.0, mat);'
for method, value in [('translate', '1e308'), ('rotate', '1e308'), ('scale', '1e200')]:
    value = format(Decimal(value), 'f') + '.0'
    refuses(method + '-overflow', shape + f' s.{method}({value}, {value}, {value}); s.{method}({value}, {value}, {value});', 'accumulated')
tiny = format(Decimal('1e-200'), 'f')
refuses('scale-underflow', shape + f' s.scale({tiny}, {tiny}, {tiny}); s.scale({tiny}, {tiny}, {tiny});', 'accumulated')
for width, height in [(0, 8), (-1, 8), (2147483647, 8), (8192, 8192), (4294967297, 8), (8, 4294967297), (-4294967295, 8)]:
    refuses('image-size', f'scene.camera(45.0, 0.0, 0.0, 5.0, 0.0, 0.0, 0.0); scene.render_png("{work.as_posix()}/bad.png", {width}, {height});', 'image dimensions exceed limits')
print('compiled DSL refusal checks passed')

refuses('material-limit', 'material();\n' * 4097, 'object limit exceeded')
refuses('shape-limit', 'let mat = material();\n' + 'scene.box(1.0, 1.0, 1.0, mat);\n' * 4097, 'object limit exceeded')
print('scene allocation limit checks passed')

refuses('name-limit', shape + ' s.name("' + 'x' * 1025 + '");', 'shape name exceeds')

# Repeated indexing returns the same immutable row; collection arena growth must
# not invalidate a handle saved from an earlier collection.
body = '''let keys = read_keyframes_csv("tests/data/keyframes.csv");
let first = keys.at(0);
for (i in range(0, 10000)) {
  if (first.x() != keys.at(0).x()) { scene.background(-1.0, 0.0, 0.0); }
}
for (i in range(0, 100)) { read_keyframes_csv("tests/data/keyframes.csv"); }
scene.background(first.r(), first.g(), first.b());'''
source = work / 'stable-rows.lms'; bytecode = work / 'stable-rows.dslb'
source.write_text(f'p = _dsl_ schema="{root}/schema/lmcad.dsl.json";\n{body}\nend _dsl_\nsavedsl(p, "{bytecode.as_posix()}");\n')
subprocess.run(compiler_cmd(compiler)+[ str(source)], check=True, stdout=subprocess.DEVNULL)
subprocess.run([host, '--dsl', str(bytecode)], check=True, stdout=subprocess.DEVNULL)
for index in [-1, 2]:
    refuses('keyframe-index', f'let keys = read_keyframes_csv("tests/data/keyframes.csv"); keys.at({index});', 'keyframe index out of range')
refuses('keyframe-collection-limit', 'for (i in range(0, 4097)) { read_keyframes_csv("tests/data/keyframes.csv"); }', 'object limit exceeded')
