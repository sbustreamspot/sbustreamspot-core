#!/usr/bin/env python

import sys
import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict

def plot_clustering_objective(C_r, k_r):
    aps_m = [] 
    stds_m = []
    for train_pct in [25, 50, 75, 100]:
        with open('pr_' + str(train_pct) + '.txt', 'r') as f:
            for line in f:
                fields = line.strip().split('\t')
                C = int(fields[0])
                if C != C_r:
                    continue
                k = int(fields[1])
                if k != k_r:
                    continue
                ap_mean = float(fields[202])
                ap_std = float(fields[203])
                aps_m.append((ap_mean, train_pct))
                stds_m.append((ap_std, train_pct))

    plt.figure()
    plt.hold(True)
    plt.rcParams.update({'axes.titlesize': 'small'})
    aps_m = sorted(aps_m, key=lambda x: x[1])
    stds_m = sorted(stds_m, key=lambda x: x[1])
    plt.errorbar([x[1] for x in aps_m],
                 [x[0] for x in aps_m],
                 yerr=[x[0] for x in stds_m],
                 linestyle='--', ecolor='r', color='b', marker='+', alpha=0.7)
    #plt.title('C = ' + str(C_r) + ', k = ' + str(k_r))
    plt.xlabel("Train %")
    plt.xticks([25, 50, 75, 100])
    plt.xlim([20,105])
    plt.ylim([0.0, 1.0])
    plt.ylabel("AP")
    plt.savefig('ap_vs_train_C' + str(C_r) + '_k' + str(k_r) + '.pdf',
                bbox_inches='tight')
    plt.clf()
    plt.close()

if __name__ == "__main__":
    C_r = int(sys.argv[1])
    k_r = int(sys.argv[2])
    plot_clustering_objective(C_r, k_r)
