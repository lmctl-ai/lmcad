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
