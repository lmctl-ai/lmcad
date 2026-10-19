"""The adapter must work outside the original checkout layout."""
import os
from pathlib import Path
import shutil
import subprocess
import sys

work = Path(sys.argv[1]) / 'relocated-clone'
(work / 'scripts').mkdir(parents=True)
for name in ['lmscript-mono.sh', 'lmscript-mono.mjs']:
    shutil.copy2(Path('scripts') / name, work / 'scripts' / name)
source = work / 'input.lms'
schema = Path('schema/lmcad.dsl.json').resolve()
body = f'''# fake = _dsl_ schema="missing";
p = _dsl_ schema="{schema}";
# end _dsl_ inside a comment
let mat = material();
scene.box(1.0, 1.0, 1.0, mat).name("end _dsl_");
end _dsl_
savedsl(p, "output.dslb");
'''
source.write_bytes(body.replace('\n', '\r\n').encode())
command = [str(work / 'scripts/lmscript-mono.sh'), str(source)]
run = subprocess.run(command, cwd=work, capture_output=True, text=True)
assert run.returncode == 0, run.stderr
assert (work / 'output.dslb').read_bytes()[:6] == b'DSLB\x00\x04'
for text in [body.replace('savedsl(p,', 'savedsl(other,'), body + body]:
    source.write_text(text)
    run = subprocess.run(command, cwd=work, capture_output=True, text=True)
    assert run.returncode == 2, run
print('relocated adapter handles delimiters and refuses ambiguous output/blocks')
