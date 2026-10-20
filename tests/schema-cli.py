"""Exact shared schema-summary format and strict CLI combinations, for the selected host."""
from pathlib import Path
import subprocess,sys
host=sys.argv[1]
def call(*args): return subprocess.run([host,*args],capture_output=True,check=True).stdout
sigs=call('--dsl-signatures').decode().splitlines()
expected=[s for s in sigs if s.startswith(('context ','fn '))]
groups={}
for s in sigs:
 if s.startswith('method '):
  body=s[7:];groups.setdefault(body.split('.')[0],[]).append(body)
for owner,methods in groups.items():
 expected.append(owner+':');expected.extend('  '+method for method in methods)
summary=call('--dsl-schema')
assert summary.decode()=='\n'.join(expected)+'\n','summary differs from shared canonical format'
assert summary==call('--dsl-schema'),'unstable summary ordering'
assert call('--dsl-schema','--json')==Path('schema/lmcad.dsl.json').read_bytes()
for args in [('--schema',),('--schema','--json'),('--json',),('--json','--dsl-schema'),('--dsl-schema','--dsl','missing'),('--dsl-schema','--json','extra'),('--dsl-schema','schema/lmcad.dsl.json'),('--dsl','missing','--dsl-schema'),('--dsl','missing','--json')]:
 r=subprocess.run([host,*args],capture_output=True)
 assert r.returncode==2 and not r.stdout,(args,r)
print(f'{len(sigs)} canonical members, exact schema bytes, stable shared summary format and invalid flags verified')
