#!/usr/bin/env python

import medoids
import random
import sys
from collections import Counter
from scipy.stats import entropy
import matplotlib
# Force matplotlib to not use any Xwindows backend.
matplotlib.use('Agg')
import matplotlib.pyplot as plt

NUM_SCENARIO_GRAPHS = 100   # number of graphs in each scenario
NUM_TRAIN_SCENARIOS = 5     # number of training (non-attack) scenarios
ANOMALY = 5                 # attack scenario ID
NUM_TOTAL_GRAPHS = NUM_SCENARIO_GRAPHS * (NUM_TRAIN_SCENARIOS + 1)

random.seed(10)

LABELS = {}                 # map graph ID to scenario label
for i in range(NUM_TRAIN_SCENARIOS+1):
    for j in range(NUM_SCENARIO_GRAPHS):
        LABELS[100*i + j] = i

def get_train_test_ids(pct_train):
    num_graphs_train = int(pct_train * NUM_SCENARIO_GRAPHS) 
    train_ids = []
    test_ids = []
    for i in range(NUM_TRAIN_SCENARIOS):
        for j in range(num_graphs_train):
            train_ids.append(100*i + j)
        for j in range(num_graphs_train, NUM_SCENARIO_GRAPHS):
            test_ids.append(100*i + j)
    test_ids.extend(range(500,600))
    return train_ids, test_ids

pct_train = float(sys.argv[1])
assert pct_train >= 0.0 and pct_train <= 1.0

for C in [25, 50, 100, 150, 200, 300, 500]:
    print 'Running for C =', C
    train_gids, test_gids = get_train_test_ids(pct_train)
    
    # read pairwise graph distances for given C
    dists = [[-1 for i in range(NUM_TOTAL_GRAPHS)]
              for j in range(NUM_TOTAL_GRAPHS)]
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

    # cluster training graphs
    diam, meds = medoids.k_medoids_iterspawn(points=train_gids, k=6,
                                             dists=dists, spawn=5,
                                             iteration=20, verbose=0)
    nclusters = len(meds)
    print '\t' + str(nclusters) + ' clusters for C:', C

    anomaly_scores = [] # list of tuples (bool, float) => (is_anomaly, anomaly_score)

    # compute anomaly scores for train gids (= dist to cluster medoid)
    centers = []
    puritysum = 0.0
    for center, gids in meds.itervalues():
        centers.append(center)
        c = Counter([LABELS[g] for g in gids])
        print '\tCluster label frequencies:', c
        puritysum += c.most_common(1)[0][1]
        for gid in gids:
            anomaly_scores.append((False, dists[center][gid]))
    print '\tCluster purity:', puritysum / 600.0

    # evaluate clustering purity

    assert len(anomaly_scores) == pct_train * \
                                  NUM_SCENARIO_GRAPHS * NUM_TRAIN_SCENARIOS

    # compute anomaly scores for test gids (= dist to nearest cluster medoid)
    for gid in test_gids:
        center_dists = sorted([(center/100, dists[gid][center])
                                for center in centers],
                               key=lambda x: x[1])
        min_center, min_dist = center_dists[0]
        if LABELS[gid] == ANOMALY:
            anomaly_scores.append((True, min_dist))
            #print 'Anomaly:', min_center, min_dist,
            #print [(center, dists[gid][center]) for center in centers] 
        else:
            anomaly_scores.append((False, min_dist))
            #print 'Not anomaly:', min_dist
    
    assert len(anomaly_scores) == NUM_TOTAL_GRAPHS

    # sort scores by anomalousness
    anomaly_scores = sorted(anomaly_scores, key=lambda x: x[1], reverse=True)
    prcurve = []
    prev_r = 0.0
    average_p = 0.0
    for threshold_idx in range(1, len(anomaly_scores)):
        threshold = anomaly_scores[threshold_idx][1]

        # anomaly classification with this threshold
        anomalies = [t[0] for t in anomaly_scores[:threshold_idx]] # pred = True
        normals = [t[0] for t in anomaly_scores[threshold_idx:]] # pred = False

        # tp, fp, precision and recall
        tp = sum([1 for i in anomalies if i == True])
        fp = sum([1 for i in anomalies if i == False])
        fn = sum([1 for i in normals if i == True])
        #print tp, tp + fn, tp + fp
        p = float(tp) / (tp + fp)
        r = float(tp) / (tp + fn)

        average_p += p * (r - prev_r)
        prev_r = r

        print '\tThreshold:', threshold, 'Precision:', p, 'Recall:', r

        prcurve.append((r,p))

    #prcurve = sorted(prcurve, key=lambda x: x[0])
    plt.figure()
    plt.rcParams.update({'axes.titlesize': 'small'})
    plt.ylim([0.0, 1.05])
    plt.plot([x[0] for x in prcurve],
             [x[1] for x in prcurve], #marker='+', markersize=4,
             linestyle='-', color='r', alpha=0.7)
    plt.title(r'Train % = ' + str(pct_train * 100) + r'%, ' +\
              r'$C = ' + str(C) + r'$, max diameter = ' + str(diam) + \
              r', $k =' + str(nclusters) + r'$' + \
              r', AP =' + "{:3.3f}".format(average_p))
    plt.xlabel('recall')
    plt.ylabel('precision')
    plt.savefig('prcurve_C' + str(C) +\
                '_pct' + str(pct_train) +\
                '.pdf',
                 bbox_inches='tight')
    plt.close()
