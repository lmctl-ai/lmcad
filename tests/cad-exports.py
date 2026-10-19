"""Closed, consistently oriented STL surfaces approximate the analytic BRep volumes."""
from collections import Counter
from pathlib import Path
import math,struct
expected={'plate-with-holes':85.3814168308665,'brick':2548.8618743534171,'vase':1708.2677036376435,
          'section-view':576.0,'mug':344.08933973285542}
for name,exact in expected.items():
 data=Path('build',name+'.stl').read_bytes();count=struct.unpack_from('<I',data,80)[0]
 edges=Counter();volume=0
 for i in range(count):
  record=struct.unpack_from('<12f',data,84+50*i)
  assert all(math.isfinite(x) for x in record),name
  a,b,c=[tuple(record[j:j+3]) for j in (3,6,9)]
  for p,q in ((a,b),(b,c),(c,a)): edges[(p,q)]+=1
  volume+=(a[0]*(b[1]*c[2]-b[2]*c[1])+a[1]*(b[2]*c[0]-b[0]*c[2])+a[2]*(b[0]*c[1]-b[1]*c[0]))/6
 assert all(n==1 and edges[(q,p)]==1 for (p,q),n in edges.items()),(name,'open/nonmanifold/inconsistent edges')
 tolerance=0.02 if name in ('mug','vase') else 0.01
 assert abs(volume-exact)/exact<tolerance,(name,volume,exact)
 print(f'{name}: closed oriented STL; mesh volume {volume:.6f}, BRep {exact:.6f} (within {tolerance:.0%})')
