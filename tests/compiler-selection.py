"""Compiler selection validates emitted bytecode and rejects absent or old compilers."""
import os
from pathlib import Path
import subprocess
import sys

root = Path.cwd()
work = Path(sys.argv[1]) / 'compiler-selection'
work.mkdir()
fake = work / 'lmscript'
real = os.environ['LMCAD_REAL_COMPILER']
fake.write_text('''#!/usr/bin/env python3
import os,subprocess,sys
from pathlib import Path
r=subprocess.run([os.environ['TEST_COMPILER'],*sys.argv[1:]])
if r.returncode: sys.exit(r.returncode)
if sys.argv[1] != '--dsl-signatures':
 p=Path(sys.argv[2])
 if os.environ['TEST_MODE']=='v4':
  b=bytearray(p.read_bytes());b[5]=4;p.write_bytes(b)
 elif os.environ['TEST_MODE']=='missing': p.unlink()
''')
fake.chmod(0o755)
env = dict(os.environ, PATH=str(work)+os.pathsep+os.environ['PATH'], TEST_COMPILER=real)
env.pop('LMSCRIPT_BIN', None)
for mode in ['v4', 'missing']:
    env['TEST_MODE'] = mode
    run = subprocess.run([str(root/'scripts/select-compiler.sh')],env=env,capture_output=True,text=True)
    assert run.returncode == 2 and 'install @lmctl-ai/lmctl' in run.stderr, (mode,run)
env['TEST_MODE'] = 'v5'
run = subprocess.run([str(root/'scripts/select-compiler.sh')],env=env,capture_output=True,text=True,check=True)
assert run.stdout.strip() == str(fake), run
env['LMSCRIPT_BIN'] = '/explicit/compiler'
assert subprocess.check_output([str(root/'scripts/select-compiler.sh')],env=env,text=True).strip() == '/explicit/compiler'
print('compiler selection accepts v5 output, rejects v4/missing output, and honors explicit selection')
