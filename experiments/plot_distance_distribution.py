#!/usr/bin/env python

import random
import sys
import numpy as np
import matplotlib.pyplot as plt
import math

SEED = 42
NUM_TRIALS = 10
NUM_SCENARIO_GRAPHS = 100
NUM_TOTAL_GRAPHS = 600
TRAIN_SCENARIOS = [0, 1, 2, 4, 5]
ATTACK_GIDS = set(range(300,400))
NORMAL_GIDS = set(range(0,300) + range(400,600))

random.seed(SEED)
np.random.seed(SEED)

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

def plot_clustering_objective(train_fraction):
    entropies = []
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

        num_train_samples = len(TRAIN_SCENARIOS) * \
                            (train_fraction * NUM_SCENARIO_GRAPHS)
        hists = []
        binedges = np.arange(0.0, 1.1, 0.1)
        for trial_num in range(NUM_TRIALS):
            train_gids, test_gids = get_train_test_ids(train_fraction)

            ds = []
            for g1 in train_gids:
                for g2 in train_gids:
                    if g1 != g2:
                        ds.append(all_dists[g1][g2])
            
            hist, edges = np.histogram(ds, bins=binedges)
            hists.append(hist)

        means = np.mean(np.array(hists), axis=0)
        stds = np.std(np.array(hists), axis=0)
        means /= num_train_samples ** 2
        stds /= num_train_samples ** 2
        print C
        plt.figure()
        plt.bar(left=binedges[:-1], height=means, width=0.1, color='b', alpha=0.7,
                yerr=stds, ecolor='r')
        plt.xticks(binedges)
        plt.ylim([0.0, 1.0])
        plt.xlabel('Distance')
        plt.ylabel('Count')
        plt.savefig('distance_histogram_C' + str(C) + \
                    '_pct' + str(int(train_fraction * 100)) + \
                    '.pdf',
                    bbox_inches='tight')
        plt.clf()
        plt.close()

        entropy = 0.0
        for m in means:
            if m != 0.0:
                entropy += -m * math.log(m,2)
        entropies.append(entropy)

    xs = [str(x) for x in [1, 2, 3, 5, 10, 25, 50, 100, 150, 200, 300, 500]]
    plt.figure()
    plt.plot(range(len(entropies)), entropies, marker='+', linestyle='-', alpha=0.7)
    plt.xlabel('C')
    plt.ylabel('Entropy')
    plt.xlim(0,len(entropies)-1)
    plt.xticks(range(len(entropies)), xs)
    plt.savefig('entropy_pct' + str(int(train_fraction * 100)) + '.pdf',
                bbox_inches='tight')
    plt.clf()
    plt.close()

if __name__ == "__main__":
    train_fraction = float(sys.argv[1])
    plot_clustering_objective(train_fraction)
