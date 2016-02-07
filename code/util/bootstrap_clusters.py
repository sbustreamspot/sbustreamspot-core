#!/usr/bin/env python

import medoids
import random
import sys
import numpy as np

SEED = 50
NUM_SCENARIO_GRAPHS = 100
NUM_TOTAL_GRAPHS = 600
TRAIN_SCENARIOS = [0, 1, 2, 4, 5]
ATTACK_GIDS = set(range(300,400))
NORMAL_GIDS = set(range(0,300) + range(400,600))

random.seed(SEED)
np.random.seed(SEED)

def get_train_test_ids(pct_train, outliers):
    num_graphs_train = int(pct_train * NUM_SCENARIO_GRAPHS) 
    train_ids = []
    test_ids = []
    for i in TRAIN_SCENARIOS:
        scenario_gids = range(100*i, 100*i + NUM_SCENARIO_GRAPHS)

        np.random.shuffle(scenario_gids)
        train_gids = scenario_gids[:num_graphs_train]
        test_gids = scenario_gids[num_graphs_train:]

        train_ids.extend(train_gids)
        test_ids.extend(test_gids)

    train_gids = [g for g in train_gids if g not in outliers]
    test_gids = [g for g in test_gids if g not in outliers]
    test_ids.extend(ATTACK_GIDS)
    return train_ids, test_ids

def bootstrap_clusters(similarity_file, nclusters, train_fraction):
    points = range(NUM_TOTAL_GRAPHS)
    all_dists = [[0 for i in points] for j in points]
    with open(similarity_file, 'r') as f:
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

    outliers = set([])
    for g1 in points:
        dists = []
        class_g1 = g1/100
        for g2 in points:
            class_g2 = g2/100
            if class_g1 != class_g1:
                continue
            dists.append(all_dists[g1][g2])
        mean_dist = np.mean(dists)
        if mean_dist > 0.8:
            outliers.add(g1)

    train_gids, test_gids = get_train_test_ids(train_fraction, outliers)

    diam, meds = medoids.k_medoids_iterspawn(train_gids, spawn=5,
                                             k=nclusters, dists=all_dists,
                                             iteration=20, verbose=0)

    threshold = {}
    all_cdists = []
    for center, gids in meds.itervalues():
        cdists = [all_dists[center][g] for g in gids]
        all_cdists.extend(cdists)
        mean_dist = np.mean(cdists)
        std_dist = np.std(cdists)
        threshold[center] = mean_dist + 3. * std_dist # P(>) <= 10%
    mean_all_cdists = np.mean(all_cdists)
    std_all_cdists = np.std(all_cdists)
    global_threshold = mean_all_cdists + 3. * std_all_cdists

    print str(nclusters) + '\t' + "{:3.4f}".format(global_threshold) 
    for center, gids in meds.itervalues():
        print "{:3.4f}".format(threshold[center]) + '\t',
        print '\t'.join([str(g) for g in gids])

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print 'Usage: python', sys.argv[0], '<sim_file> <train_frac> <nclusters>'
        sys.exit(-1)
    similarity_file = sys.argv[1] 
    train_fraction = float(sys.argv[2])
    assert train_fraction >= 0.0 and train_fraction <= 1.0
    nclusters = int(sys.argv[3])
    assert nclusters > 1
    bootstrap_clusters(similarity_file, nclusters, train_fraction)
