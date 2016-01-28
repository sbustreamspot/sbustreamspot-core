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
import numpy as np

NUM_SCENARIO_GRAPHS = 100   # number of graphs in each scenario
NUM_TRAIN_SCENARIOS = 5     # number of training (non-attack) scenarios
ANOMALY = 5                 # attack scenario ID
NUM_TOTAL_GRAPHS = NUM_SCENARIO_GRAPHS * (NUM_TRAIN_SCENARIOS + 1)
NUM_TRIALS = 10

random.seed(20)

LABELS = {}                 # map graph ID to scenario label
for i in range(NUM_TRAIN_SCENARIOS+1):
    for j in range(NUM_SCENARIO_GRAPHS):
        LABELS[100*i + j] = i

def get_train_test_ids(pct_train):
    num_graphs_train = int(pct_train * NUM_SCENARIO_GRAPHS) 
    train_ids = []
    test_ids = []
    for i in range(NUM_TRAIN_SCENARIOS):
        # sample pct_train gid's from each scenario
        scenario_gids = range(100*i, 100*i + NUM_SCENARIO_GRAPHS)

        np.random.shuffle(scenario_gids)
        train_gids = scenario_gids[:num_graphs_train]
        test_gids = scenario_gids[num_graphs_train:]

        train_ids.extend(train_gids)
        test_ids.extend(test_gids)

    test_ids.extend(range(500,600))
    return train_ids, test_ids

for C in [25, 50, 100, 150, 200, 300, 500]:
    #print 'Running for C =', C
    
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

    aps = []
    aucs = []
    for pct_train in [0.2, 0.4, 0.6, 0.8, 1.0]:
        for trial_num in range(NUM_TRIALS):
            train_gids, test_gids = get_train_test_ids(pct_train)

            # cluster training graphs
            diam, meds = medoids.k_medoids_iterspawn(points=train_gids,
                                                     k=int(sys.argv[1]),
                                                     dists=dists, spawn=10,
                                                     iteration=50, verbose=0)
            nclusters = len(meds)

            anomaly_scores = [] # list of tuples (bool, float) => (is_anomaly, anomaly_score)

            # compute anomaly scores for train gids (= dist to cluster medoid)
            centers = []
            puritysum = 0.0
            for center, gids in meds.itervalues():
                centers.append(center)
                c = Counter([LABELS[g] for g in gids])
                #print '\tCluster label frequencies:', c
                puritysum += c.most_common(1)[0][1]
                for gid in gids:
                    anomaly_scores.append((False, dists[center][gid]))
            #print '\tCluster purity:', puritysum / 600.0

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
            roccurve = []
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
                tn = sum([1 for i in normals if i == False])
                fn = sum([1 for i in normals if i == True])
                #print tp, tp + fn, tp + fp

                p = float(tp) / (tp + fp)
                r = float(tp) / (tp + fn)
                tpr = float(tp) / (tp + fn)
                fpr = float(fp) / (fp + tn)

                average_p += p * (r - prev_r)
                prev_r = r

                #print '\tThreshold:', threshold, 'Precision:', p, 'Recall:', r

                prcurve.append((r,p))
                roccurve.append((fpr, tpr))

            aps.append(average_p) # average over all trials
            auc = np.trapz(y=[c[1] for c in roccurve],
                           x=[c[0] for c in roccurve])
            aucs.append(auc)

            """PR plot"""
            plt.figure()
            plt.rcParams.update({'axes.titlesize': 'small'})
            plt.ylim([0.0, 1.05])
            plt.plot([x[0] for x in prcurve],
                     [x[1] for x in prcurve], #marker='+', markersize=4,
                     linestyle='-', color='r', alpha=0.7)
            plt.title(r'Train = ' + str(pct_train * 100) + r'%, ' +\
                      r'$C = ' + str(C) + \
                      r'$, max dia = ' + "{:3.3f}".format(diam) + \
                      r', $k =' + str(nclusters) + r'$' + \
                      r', AP =' + "{:3.3f}".format(average_p))
            plt.xlabel('recall')
            plt.ylabel('precision')
            plt.savefig('prcurve_C' + str(C) +\
                        '_k' + str(nclusters) +\
                        '_pct' + str(pct_train) +\
                        '.pdf',
                         bbox_inches='tight')
            plt.close()

            """ROC plot"""
            plt.figure()
            plt.rcParams.update({'axes.titlesize': 'small'})
            plt.ylim([0.0, 1.05])
            plt.plot([x[0] for x in roccurve],
                     [x[1] for x in roccurve], #marker='+', markersize=4,
                     linestyle='-', color='r', alpha=0.7)
            plt.title(r'Train = ' + str(pct_train * 100) + r'%, ' + \
                      r'$C = ' + str(C) + \
                      r'$, max dia = ' + "{:3.3f}".format(diam) + \
                      r', $k =' + str(nclusters) + r'$' + \
                      r', AUC =' + "{:3.3f}".format(auc))
            plt.xlabel('FPR')
            plt.ylabel('TPR')
            plt.savefig('roccurve_C' + str(C) +\
                        '_k' + str(nclusters) +\
                        '_pct' + str(pct_train) +\
                        '.pdf',
                         bbox_inches='tight')
            plt.close()

        print 'C=' + str(C) + '\t',
        print 'k=' + str(nclusters) + '\t',
        print 'maxdia=' + "{:3.3f}".format(diam) + '\t',
        print 'train=' + str(pct_train * 100.0) + '\t',
        print 'AP=' + "{:3.3f}".format(np.mean(aps)),
        print ' (+/-' + "{:3.3f}".format(np.std(aps)) + ')\t',
        print 'AUROC=' + "{:3.3f}".format(np.mean(aucs)),
        print ' (+/-' + "{:3.3f}".format(np.std(aucs)) + ')'
