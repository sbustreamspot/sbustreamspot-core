#!/usr/bin/env python

import sys
import numpy as np
import matplotlib.pyplot as plt

def plot_clustering_objective(train_fraction):
    ps = {}
    aps = {}
    stds = {}
    with open('pr_' + str(int(train_fraction * 100)) + '.txt', 'r') as f:
        for line in f:
            fields = line.strip().split('\t')
            C = int(fields[0])
            if C == 15:
                continue
            k = int(fields[1])
            p_mean = [float(x) for x in fields[2:102]]
            p_stds = [float(x) for x in fields[102:202]]
            ap_mean = float(fields[202])
            ap_std = float(fields[203])

            ps[C,k] = p_mean
            aps[C,k] = ap_mean
            stds[C,k] = ap_std
            
    for (C, k), precisions in ps.iteritems(): 
        plt.figure()
        plt.rcParams.update({'axes.titlesize': 'small'})
        plt.ylim([0.0, 1.05])
        plt.plot([i/100. for i in range(1,101)],
                 [p for p in precisions],
                 linestyle='-', color='r', alpha=0.7)
        plt.title('Train = ' + str(train_fraction * 100) + '%, ' + \
                  'C = ' + str(C) + \
                  ', k =' + str(k) + \
                  ', AP =' + "{:3.3f}".format(aps[C,k]) + \
                  ' (+/-' + "{:3.3f}".format(stds[C,k]) + ')')
        plt.xlabel('Recall')
        plt.ylabel('Precision')
        plt.savefig('prcurve_C' + str(C) +\
                    '_k' + str(k) +\
                    '_pct' + str(int(train_fraction * 100)) +\
                    '.pdf',
                     bbox_inches='tight')
        plt.close()


if __name__ == "__main__":
    train_fraction = float(sys.argv[1])
    plot_clustering_objective(train_fraction)
