#!/usr/bin/env python

import math
import networkx as nx
import sys

ATTACK_GIDS = range(300,400)
NORMAL_GIDS = range(0,300) + range(400,600)

b = float(sys.argv[1])
r = float(sys.argv[2])
threshold = (1/b) ** (1/r) 

# Union-find from Kruskal's algorithm:
# https://github.com/israelst/Algorithms-Book--Python/blob/master/5-Greedy-algorithms/kruskal.py
parent = dict()
rank = dict()

def make_set(vertice):
    parent[vertice] = vertice
    rank[vertice] = 0

def find(vertice):
    if parent[vertice] != vertice:
        parent[vertice] = find(parent[vertice])
    return parent[vertice]

def union(vertice1, vertice2):
    root1 = find(vertice1)
    root2 = find(vertice2)
    if root1 != root2:
        if rank[root1] > rank[root2]:
            parent[root2] = root1
        else:
            parent[root1] = root2
            if rank[root1] == rank[root2]: rank[root2] += 1
# End.

for C in [25, 50, 100]:
    print 'Clusters for C:', C, 'Threshold:', threshold
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
            sim = float(fields[2]) # cosine similarity = cos(theta)
            dists[g1][g2] = 1 - math.acos(sim)/math.pi
            dists[g2][g1] = 1 - math.acos(sim)/math.pi

    G = nx.Graph()
    for v1 in NORMAL_GIDS:
        for v2 in NORMAL_GIDS:
            if v1 == v2:
                continue

            if dists[v1][v2] >= threshold:
                G.add_edge(v1, v2)

    for i, cc in enumerate(nx.connected_components(G)):
        print 'CC ' + str(i) + ': ' + str(cc)
        cluster_anomalies = []
        for a in ATTACK_GIDS:
            for v in cc:
                if dists[a][v] >= threshold:
                    cluster_anomalies.append((a, dists[a][v]))
        if len(cluster_anomalies) > 0:
            print '\tANOMALIES:', cluster_anomalies
