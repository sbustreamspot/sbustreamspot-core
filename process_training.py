#!/usr/bin/env python

import sys
import uuid

uuidmap = {}

for line in sys.stdin:
    fields = line.strip().split('\t')
    sid = fields[0]
    tid = fields[2]
    gid = fields[5]

    if sid in uuidmap:
        newsid = uuidmap[sid]
    else:
        newsid = uuid.uuid1().hex
        uuidmap[sid] = newsid
    
    if tid in uuidmap:
        newtid = uuidmap[tid]
    else:
        newtid = uuid.uuid1().hex
        uuidmap[tid] = newtid
    
    if gid in uuidmap:
        newgid = uuidmap[gid]
    else:
        newgid = uuid.uuid1().hex
        uuidmap[gid] = newgid

    print newsid + '\t' + fields[1] + '\t',
    print newtid + '\t' + fields[3] + '\t',
    print fields[4] + '\t' + newgid
