#!/usr/bin/env python3
"""Recover a separately patched full-client source tree; reject unrecorded changes."""
from pathlib import Path
import hashlib,json,sys,tarfile,tempfile,subprocess,shutil,os
r=Path(sys.argv[1]);source=Path(__file__).resolve().parent.parent
entry=json.loads((source/'voxel/dependencies.lock.json').read_text())['luanti'];archive=source/'.local/downloads'/entry['archive']
assert hashlib.sha256(archive.read_bytes()).hexdigest()==entry['sha256']
dest=r/'client-sources/luanti';statefile=r/'client-source-state.json'
def files(base):return {str(p.relative_to(base)):hashlib.sha256(p.read_bytes()).hexdigest() for p in base.rglob('*') if p.is_file()}
if dest.exists():
 assert statefile.exists(),'Unregistered client source tree; inspect manually'
 assert files(dest)==json.loads(statefile.read_text())['files'],'Client source changed outside patch recipe; refusing replacement'
with tempfile.TemporaryDirectory(dir=r) as tmp:
 pristine=Path(tmp)
 with tarfile.open(archive) as t:t.extractall(pristine,filter='data')
 for patch in sorted((source/'client/patches').glob('*.patch')):subprocess.run(['patch','--fuzz=0','-p1','-i',str(patch)],cwd=pristine,check=True)
 expected=files(pristine)
 dest.mkdir(parents=True,exist_ok=True)
 for rel,digest in expected.items():
  path=dest/rel
  if path.is_file() and hashlib.sha256(path.read_bytes()).hexdigest()==digest:continue
  path.parent.mkdir(parents=True,exist_ok=True);pending=path.with_name(path.name+'.ps4-pending');shutil.copy2(pristine/rel,pending);os.replace(pending,path)
 assert files(dest)==expected
 statefile.write_text(json.dumps({'upstream':entry,'files':expected},indent=2)+'\n')
print('Full Luanti client source and exact patches verified')
