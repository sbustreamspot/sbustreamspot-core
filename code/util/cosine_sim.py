import sys
import math
from collections import defaultdict

g0f = sys.argv[1]
g1f = sys.argv[2]

g0 = {}
with open(g0f, 'r') as f:
    for line in f:
        fields = line.strip().split('\t')
        g0[fields[0]] = int(fields[1])
g1 = {}
with open(g1f, 'r') as f:
    for line in f:
        fields = line.strip().split('\t')
        g1[fields[0]] = int(fields[1])

dot_product = 0.0
for k0, v0 in g0.iteritems():
    for k1, v1 in g1.iteritems():
        if k0 == k1:
            #print 'C:', k0, v0, v1
            dot_product += v0 * v1
        #else:
            #print 'U:', k0, v0, k1, v1

#print dot_product
mag0 = math.sqrt(sum([v * v for v in g0.values()]))
mag1 = math.sqrt(sum([v * v for v in g1.values()]))
#print mag0, mag1
print 'Without chunking:', dot_product / mag0 / mag1

def chunkstring(string, length):
    return (string[0+i:length+i] for i in range(0, len(string), length))

chunklength = int(sys.argv[3])
g0chunked =  {}
for k, v in g0.iteritems():
    for chunk in chunkstring(k, chunklength):
        if not chunk in g0chunked:
            g0chunked[chunk] = 0
        g0chunked[chunk] += v

g1chunked = {}
for k, v in g1.iteritems():
    for chunk in chunkstring(k, chunklength):
        if not chunk in g1chunked:
            g1chunked[chunk] = 0
        g1chunked[chunk] += v

dot_product = 0.0
for k0, v0 in g0chunked.iteritems():
    if k0 in g1chunked:
        dot_product += v0 * g1chunked[k0]

#print dot_product
mag0 = math.sqrt(sum([v * v for v in g0chunked.values()]))
mag1 = math.sqrt(sum([v * v for v in g1chunked.values()]))
#print mag0, mag1
print 'With chunking:', dot_product / mag0 / mag1 
