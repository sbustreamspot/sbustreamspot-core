#!/usr/bin/env python

import medoids
from collections import Counter
from scipy.stats import entropy
import random

random.seed(42)

labels = {}
for i in range(100):
    labels[i] = 0           # gid 0     -   99
    labels[i+100] = 1       # gid 100   -   199
    labels[i+200] = 2       # gid 200   -   299
    labels[i+300] = 3       # gid 300   -   399
    labels[i+400] = 4       # gid 400   -   499
    labels[i+500] = 5       # gid 500   -   599

for C in [25, 50, 100, 150, 200, 300, 500]:
    print 'Clusters for C:', C
    points = range(0,600)
    dists = [[0 for i in range(600)] for j in range(600)]
    with open('sim_results_C' + str(C) + '.txt', 'r') as f:
        next(f)
        next(f)
        next(f)
        next(f)
        for line in f:
            fields = line.strip().split('\t')
            g1 = int(fields[0])
            g2 = int(fields[1])
            sim = float(fields[2])
            dists[g1][g2] = 1.0 - sim
            dists[g2][g1] = 1.0 - sim

        diam, meds = medoids.k_medoids(points, k=6, dists=dists, iteration=20,
                                       verbose=0)

        puritysum = 0.0
        for center, gids in meds.itervalues():
            c = Counter([labels[g] for g in gids])
            print '\tCluster label frequencies:', c
            puritysum += c.most_common(1)[0][1] 
        print '\tCluster purity:', puritysum / 600.0
    print
