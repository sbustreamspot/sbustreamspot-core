#!/usr/bin/env python

import sys
import numpy as np
import matplotlib.pyplot as plt

def plot_clustering_objective(train_fraction):
    Cs = set([])
    ks = set([])
    aps_m = {}
    aps_s = {}
    with open('pr_' + str(int(train_fraction * 100)) + '.txt', 'r') as f:
        for line in f:
            fields = line.strip().split('\t')
            C = int(fields[0])
            if C == 15:
                continue
            k = int(fields[1])
            #p_mean = [float(x) for x in fields[2:102]]
            #p_stds = [float(x) for x in fields[102:202]]
            ap_mean = float(fields[202])
            ap_std = float(fields[203])

            Cs.add(C)
            ks.add(k)
            aps_m[C,k] = ap_mean
            aps_s[C,k] = ap_std

    xs = list(sorted(Cs))
    ys = list(sorted(ks))
    x_surf, y_surf = np.meshgrid(xs, ys)
    z_surf = np.zeros((len(ys), len(xs)))

    for i in range(len(ys)):
        for j in range(len(xs)):
            z_surf[i][j] = aps_m[x_surf[i][j], y_surf[i][j]]
            print x_surf[i][j], y_surf[i][j], z_surf[i][j]

    fig = plt.figure()
    plt.hold(True)
    ax = fig.gca()
    ax.pcolor(z_surf, cmap=plt.cm.Reds)
    ax.set_xticks(np.arange(len(xs))+0.5, minor=False)
    ax.set_yticks(np.arange(len(ys))+0.5, minor=False)
    ax.set_xticklabels(xs, minor=False)
    ax.set_yticklabels(ys, minor=False)
    ax.set_xlabel('Chunk Length')
    ax.set_ylabel('Number of Clusters')
    plt.savefig('ap_vs_c_k_' + str(int(train_fraction * 100)) + '.pdf',
                bbox_inches='tight')

if __name__ == "__main__":
    train_fraction = float(sys.argv[1])
    plot_clustering_objective(train_fraction)
