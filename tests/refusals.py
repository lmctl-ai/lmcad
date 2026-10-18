"""Exercise validation through compiled DSL, rather than only native helpers."""
import os
from decimal import Decimal
from pathlib import Path
import subprocess
import sys

root = Path.cwd()
work = Path(sys.argv[1])
compiler = os.environ.get('LMSCRIPT_BIN', 'lmscript')

def refuses(name, body, error):
    source = work / (name + '.lms')
    bytecode = work / (name + '.dslb')
    source.write_text(f'p = _dsl_ schema="{root}/schema/lmcad.dsl.json";\n{body}\nend _dsl_\nsavedsl(p, "{bytecode}");\n')
    subprocess.run([compiler, str(source)], check=True, stdout=subprocess.DEVNULL)
    run = subprocess.run(['build/lmcad', '--dsl', str(bytecode), '--schema', 'schema/lmcad.dsl.json'], capture_output=True, text=True)
    assert run.returncode == 3 and error in run.stderr, (name, run.returncode, run.stderr)

shape = 'let mat = material(); let s = scene.box(2.0, 2.0, 2.0, mat);'
for method, value in [('translate', '1e308'), ('rotate', '1e308'), ('scale', '1e200')]:
    value = format(Decimal(value), 'f') + '.0'
    refuses(method + '-overflow', shape + f' s.{method}({value}, {value}, {value}); s.{method}({value}, {value}, {value});', 'accumulated')
tiny = format(Decimal('1e-200'), 'f')
refuses('scale-underflow', shape + f' s.scale({tiny}, {tiny}, {tiny}); s.scale({tiny}, {tiny}, {tiny});', 'accumulated')
for value in ['1.0 / 0.0', '0.0 / 0.0']:
    refuses('nonfinite', shape + f' s.scale({value}, 1.0, 1.0);', 'must be finite')
for width, height in [(0, 8), (-1, 8), (2147483647, 8), (8192, 8192)]:
    refuses('image-size', f'scene.camera(45.0, 0.0, 0.0, 5.0, 0.0, 0.0, 0.0); scene.render_png("{work}/bad.png", {width}, {height});', 'image dimensions exceed limits')
print('compiled DSL refusal checks passed')

# The SDK checks names/arity but not callback argument types. Malformed yet
# structurally valid bytecode must never reach an unchecked object cast.
import hashlib
import struct
constants = [b'scene', b'Scene', b'box']
load_scene = b'\x06\x00\x00'
float_one = b'\x12' + struct.pack('>d', 1.0)
for bad_material in [b'\x01' + struct.pack('>i', 0), load_scene]:
    code = load_scene + float_one * 3 + bad_material + b'\x08\x00\x01\x00\x02\x04\x11'
    blob = b'DSLB' + struct.pack('>H', 3) + hashlib.sha256(Path('schema/lmcad.dsl.json').read_bytes()).digest()
    blob += struct.pack('>H', len(constants)) + b''.join(struct.pack('>H', len(c)) + c for c in constants)
    blob += struct.pack('>HI', 0, len(code)) + code
    path = work / 'bad-kind.dslb'
    path.write_bytes(blob)
    run = subprocess.run(['build/lmcad', '--dsl', str(path), '--schema', 'schema/lmcad.dsl.json'], capture_output=True, text=True)
    assert run.returncode == 3 and 'argument 4 must be Material' in run.stderr, run
print('malformed callback argument checks passed')

refuses('material-limit', 'material();\n' * 4097, 'object limit exceeded')
refuses('shape-limit', 'let mat = material();\n' + 'scene.box(1.0, 1.0, 1.0, mat);\n' * 4097, 'object limit exceeded')
print('scene allocation limit checks passed')

refuses('name-limit', shape + ' s.name("' + 'x' * 1025 + '");', 'shape name exceeds')
