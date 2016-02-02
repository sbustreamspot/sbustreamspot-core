#!/usr/bin/env python

import medoids
from collections import Counter
from scipy.stats import entropy
import random
import sys
from sklearn.metrics import silhouette_score, auc
import numpy as np
import warnings
from scipy import interp

SEED = 42
NUM_TRIALS = 10
NUM_KMEDOIDS_TRIALS = 10
NUM_SCENARIO_GRAPHS = 100
NUM_TOTAL_GRAPHS = 600
TRAIN_SCENARIOS = [0, 1, 2, 4, 5]
ATTACK_GIDS = set(range(300,400))
NORMAL_GIDS = set(range(0,300) + range(400,600))

random.seed(SEED)
np.random.seed(SEED)
np.seterr(all='ignore')
warnings.filterwarnings("ignore")

def get_train_test_ids(pct_train):
    num_graphs_train = int(pct_train * NUM_SCENARIO_GRAPHS) 
    train_ids = []
    test_ids = []
    for i in TRAIN_SCENARIOS:
        # sample pct_train gid's from each scenario
        scenario_gids = range(100*i, 100*i + NUM_SCENARIO_GRAPHS)

        np.random.shuffle(scenario_gids)
        train_gids = scenario_gids[:num_graphs_train]
        test_gids = scenario_gids[num_graphs_train:]

        train_ids.extend(train_gids)
        test_ids.extend(test_gids)

    test_ids.extend(ATTACK_GIDS)
    return train_ids, test_ids

def compute_clustering_objective(meds, X):
    labels = np.array([-1 for i in range(X.shape[0])])
    for label, (center, graphs) in meds.iteritems():
        for g in graphs:
            labels[g] = center
    return silhouette_score(X, labels, metric='precomputed', random_state=SEED)

def plot_clustering_objective(train_fraction):
    for C in [1, 2, 3, 5, 10, 15, 25, 50, 100, 150, 200, 300, 500]:
        points = range(NUM_TOTAL_GRAPHS)
        all_dists = [[0 for i in points] for j in points]
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
                all_dists[g1][g2] = 1.0 - sim
                all_dists[g2][g1] = 1.0 - sim

        for k in range(3,12):
            precisions = np.zeros((NUM_TRIALS, NUM_SCENARIO_GRAPHS))
            tprs = [[] for i in range(NUM_TRIALS)]
            fprs = [[] for i in range(NUM_TRIALS)]
            aps = []

            for trial_num in range(NUM_TRIALS):
                train_gids, test_gids = get_train_test_ids(train_fraction)
                train_idx = {}
                train_gid = {}
                for idx, gid in enumerate(train_gids):
                    train_idx[gid] = idx
                    train_gid[idx] = gid

                # get pairwise distances between training graphs
                X = np.zeros((len(train_gids), len(train_gids)))
                for g1 in train_gids:
                    for g2 in train_gids:
                        X[train_idx[g1]][train_idx[g2]] = all_dists[g1][g2]

                best_obj = -1
                best_clustering = None
                for i in range(NUM_KMEDOIDS_TRIALS):
                    diam, meds = medoids.k_medoids(range(len(train_gids)),
                                                   k=k, dists=X,
                                                   iteration=100, verbose=0)
                    try:
                        obj = compute_clustering_objective(meds, X)
                        if obj > best_obj:
                            best_obj = obj
                            best_clustering = meds
                    except:
                        continue

                centers = [train_gid[center] for center, gids in meds.itervalues()]

                # training data scores
                anomaly_scores = [(False,
                                   all_dists[train_gid[center]][train_gid[gid]])
                                  for center, gids in meds.itervalues()
                                  for gid in gids]
                assert len(anomaly_scores) == len(train_gids)

                # test data scores
                anomaly_scores += [(gid in ATTACK_GIDS,
                                   min([all_dists[center][gid]
                                        for center in centers]))
                                   for gid in test_gids]

                assert len(anomaly_scores) == NUM_TOTAL_GRAPHS

                anomaly_scores = sorted(anomaly_scores, key=lambda x: x[1],
                                        reverse=True)

                prev_r = 0.0
                average_p = 0.0
                anomaly_idx = 0.0
                for threshold_idx in range(0, len(anomaly_scores)-1):
                    label, threshold = anomaly_scores[threshold_idx]
                    if label == False:
                        continue

                    anomaly_idx += 1.0

                    pred_true = [t[0] for t in anomaly_scores[:threshold_idx+1]]
                    pred_false = [t[0] for t in anomaly_scores[threshold_idx+1:]]

                    tp = sum([1 for i in pred_true if i == True])
                    fp = sum([1 for i in pred_true if i == False])
                    tn = sum([1 for i in pred_false if i == False])
                    fn = sum([1 for i in pred_false if i == True])

                    p = float(tp) / (tp + fp)
                    r = float(tp) / (tp + fn)
                    tpr = float(tp) / (tp + fn)
                    fpr = float(fp) / (fp + tn)

                    assert r == anomaly_idx/NUM_SCENARIO_GRAPHS

                    average_p += p * (r - prev_r)
                    prev_r = r
                    
                    precisions[trial_num][anomaly_idx-1] = p
                # end for threshold_idx

                """
                for threshold_idx in range(1, len(anomaly_scores)):
                    pred_true = [t[0] for t in anomaly_scores[:threshold_idx+1]]
                    pred_false = [t[0] for t in anomaly_scores[threshold_idx+1:]]
                    
                    tp = sum([1 for i in pred_true if i == True])
                    fp = sum([1 for i in pred_true if i == False])
                    tn = sum([1 for i in pred_false if i == False])
                    fn = sum([1 for i in pred_false if i == True])

                    p = float(tp) / (tp + fp)
                    r = float(tp) / (tp + fn)
                    tpr = float(tp) / (tp + fn)
                    fpr = float(fp) / (fp + tn)

                    tprs[trial_num].append(tpr)
                    fprs[trial_num].append(fpr)
                # end for threshold_idx
                """

                aps.append(average_p)
            # end for trial_num

            """
            # compute macro-average ROC curve
            all_fpr = np.unique(np.concatenate([fprs[i] for i in range(NUM_TRIALS)]))
            mean_tpr = np.zeros_like(all_fpr)
            for i in range(NUM_TRIALS):
                print fprs[i], tprs[i]
                mean_tpr += interp(all_fpr, fprs[i], tprs[i])
                print mean_tpr
            mean_tpr /= NUM_TRIALS
            mean_auc = auc(all_fpr, mean_tpr)
            """

            # compute average precisions across NUM_TRIALS trials
            means = np.mean(precisions, axis=0)
            stds = np.std(precisions, axis=0)

            print str(C) + '\t' + str(k) + '\t',
            for anomaly_idx in range(NUM_SCENARIO_GRAPHS):
                print str(means[anomaly_idx]) + '\t', 
            for anomaly_idx in range(NUM_SCENARIO_GRAPHS):
                print str(stds[anomaly_idx]) + '\t', 
            print str(np.mean(aps)) + '\t' + str(np.std(aps)) + '\t'

            """
            for f, t in zip(all_fpr, mean_tpr):
                print str(f) + ',' + str(t) + '\t',
            print str(mean_auc)
            """

if __name__ == "__main__":
    train_fraction = float(sys.argv[1])
    plot_clustering_objective(train_fraction)
