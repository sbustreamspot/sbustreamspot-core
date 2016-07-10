#!/usr/bin/env python
import json

flines = open('test.json', 'r').readlines()
glines = open('results.json', 'r').readlines()

for f, g in zip(flines, glines):
    fjson = json.loads(f[:-1])
    gjson = json.loads(g[:-1])
    for k, v in fjson.iteritems():
        if k != 'timestamp':
            assert fjson[k] == gjson[k]
