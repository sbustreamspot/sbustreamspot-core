#!/usr/bin/env python

import sys
from sklearn.metrics import average_precision_score, roc_auc_score
import matplotlib.pyplot as plt
import numpy as np
import glob

BYTES_PER_EDGE = 5 * 4 # 4-bytes per tuple element

if __name__ == "__main__":
    pcts = []
    s = []
    p = []
    g = []
    h = []
    for par in [100]:
        for pct in [75, 50, 25, 15, 10, 5]:
            filename = "anomaly_scores_C50_k10_pct75_thresh10_par" +\
                        str(par) + "_L1000_cache" + str(pct) +\
                        "_nooutliers.txt"
            with open(filename, 'r') as f:
                for i in range(12):
                    f.readline()
                fields = f.readline().strip().split(' ')

                ngraphs_total = 0
                nedges_total = 0
                for field in fields:
                    t = field.split(',')
                    gid = int(t[0])
                    nedges = int(t[1])

                    if nedges > 0:
                        ngraphs_total += 1
                        nedges_total += nedges

            sketch_memory = 1/8. * 1000 * ngraphs_total / 1024.
            projection_memory = 4. * 1000 * ngraphs_total / 1024.
            graph_memory = nedges_total * BYTES_PER_EDGE / 1024. / 1024.

            pcts.append(pct/100. * 25)
            s.append(sketch_memory)
            p.append(projection_memory)
            g.append(graph_memory)
            h.append(ngraphs_total)

    plt.rcParams.update({'axes.titlesize': 'large', 'axes.labelsize': 'large'})
    plt.figure()
    plt.hold(True)

    #plt.bar(np.arange(len(pcts)), s, color='r',
    #        label='Sketches', width=0.8, align='center', alpha=0.7,
    #        bottom=p)
    #plt.bar(np.arange(len(pcts)), p, color='b',
    #        label='Projection Vectors', width=0.8, align='center', alpha=0.7,
    #        )
    plt.bar(np.arange(len(pcts)), g, color='g',
            label='Graphs', width=0.8, align='center', alpha=0.7)
    plt.xticks(np.arange(len(pcts)), ["{:3.2f}".format(pct) for pct in pcts])
    plt.ylabel('Memory usage (MB)')
    plt.xlabel('Maximum number of edges in memory (in millions)')
    #plt.legend(loc='upper right')
    plt.savefig('memory_usage.pdf', bbox_inches='tight')
