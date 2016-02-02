#!/usr/bin/env python

import medoids
from collections import Counter
from scipy.stats import entropy
import random
import sys
from sklearn.metrics import silhouette_score
import numpy as np
import warnings

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
    for C in [1, 2, 3, 5, 10, 25, 50, 100, 150, 200, 300, 500]:
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
            objs = []
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

                """
                for i in range(X.shape[0]):
                    for j in range(X.shape[1]):
                        assert X[i][j] == all_dists[train_gid[i]][train_gid[j]]
                """

                best_obj = -1
                for i in range(NUM_KMEDOIDS_TRIALS):
                    diam, meds = medoids.k_medoids(range(len(train_gids)),
                                                   k=k, dists=X,
                                                   iteration=100, verbose=0)
                    obj = compute_clustering_objective(meds, X)
                    if obj > best_obj:
                        best_obj = obj

                objs.append(best_obj)

            mean_obj = np.mean(objs) # mean over NUM_TRIALS, given C, k
            std_obj = np.std(objs)

            print str(C) + '\t' + str(k) + '\t',
            #print "{:3.4f}".format(mean_obj) + '\t',
            #print "{:3.4f}".format(std_obj)
            print str(mean_obj) + '\t',
            print str(std_obj)

if __name__ == "__main__":
    train_fraction = float(sys.argv[1])
    plot_clustering_objective(train_fraction)
