#!/usr/bin/env python3
"""Check first-party current Markdown paths and GitHub-style heading anchors."""
from pathlib import Path
from urllib.parse import unquote,urlsplit
import re
r=Path(__file__).resolve().parents[1]
files=[r/'README.md',r/'docs/CURRENT-STATUS.md',r/'docs/PUBLIC-FIRST-INSTALL.md',*sorted((r/'docs/current').rglob('*.md'))]
fail=[];count=0
for p in files:
 for target in re.findall(r'\]\(([^\s)]+)\)',p.read_text()):
  u=urlsplit(target)
  if u.scheme or u.netloc:continue
  dst=(p.parent/unquote(u.path)).resolve() if u.path else p
  count+=1
  if not dst.exists():fail.append(f'{p.relative_to(r)}: missing {target}');continue
  if u.fragment and dst.suffix=='.md':
   anchors=[];seen={}
   for title in re.findall(r'^#{1,6}\s+(.+?)\s*#*$',dst.read_text(),re.M):
    slug=re.sub(r'[^\w\- ]','',title.lower()).replace(' ','-'); n=seen.get(slug,0);seen[slug]=n+1
    anchors.append(slug+(f'-{n}' if n else ''))
   if unquote(u.fragment) not in anchors:fail.append(f'{p.relative_to(r)}: missing anchor {target}')
assert not fail,'\n'.join(fail)
print(f'PASS {count} current first-party local links/anchors across {len(files)} documents; external URL availability not claimed')
