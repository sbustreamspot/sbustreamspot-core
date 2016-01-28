#!/usr/bin/env python

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

for C in [25, 50, 100, 150, 200, 300, 500]:
    print 'Clusters for C:', C
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

    # plot distribution of distances from attack graphs to normal graphs
    distribution = []
    for attack_gid in range(500,600):
        distribution.append(min([dists[attack_gid][normal_gid]
                                 for normal_gid in range(0,500)]))
    plt.hist(distribution, alpha=0.7, bins=10)
    plt.title('Attack-graph Min-Distance')
    plt.xlabel('distance')
    plt.ylabel('count')
    plt.savefig('attack_graph_distance_distribution_C' + str(C) + '.pdf',
                bbox_inches='tight')
    plt.clf()
    plt.close()
    
    # plot distribution of distances between normal graphs
    distribution = []
    for gid1 in range(0,500):
        distribution.append(min([dists[gid1][gid2]
                                 for gid2 in range(0,500)]))
    plt.hist(distribution, alpha=0.7, bins=10)
    plt.title('Normal-graph Min-distance')
    plt.xlabel('distance')
    plt.ylabel('count')
    plt.savefig('normal_graph_distance_distribution_C' + str(C) + '.pdf',
                bbox_inches='tight')
    plt.close()
