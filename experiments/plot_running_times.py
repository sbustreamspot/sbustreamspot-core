#!/usr/bin/env python

import sys
from sklearn.metrics import average_precision_score, roc_auc_score
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({'axes.titlesize': 'large', 'axes.labelsize': 'large'})
plt.figure()
plt.hold(True)

xs = [2000, 1000, 100]
sketch_update = [166, 58, 4]
cluster_update = [8, 2, 0]
times = [sketch_update, cluster_update]

#plt.plot(xs, sketch_update, 'r-', marker='+', label='Sketch Update', alpha=0.7)
#plt.plot(xs, cluster_update, 'b-', marker='+', label='Cluster Update', alpha=0.7)
plt.bar(np.arange(3), cluster_update, color='b', label='Cluster Update',
        alpha=0.7, width=0.8, align='center')
plt.bar(np.arange(3), sketch_update, color='r', label='Sketch Update',
        alpha=0.7, bottom=cluster_update, width=0.8, align='center')
plt.xticks(np.arange(3), xs)
plt.ylabel('Running Time (us)')
plt.xlabel('Sketch Size (bits)')
plt.legend(loc='upper right')
plt.savefig('running_times_plot.pdf', bbox_inches='tight')
