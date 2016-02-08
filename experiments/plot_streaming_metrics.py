#!/usr/bin/env python

import sys
from sklearn.metrics import average_precision_score, roc_auc_score
import matplotlib.pyplot as plt
import numpy as np

UNSEEN = -2
ANOMALY = -1

def compute_streaming_metrics(metrics_file, bootstrap_file, spacing=1):
    # collect training gids
    training_gids = []
    with open(bootstrap_file, 'r') as f:
        fields = f.readline().strip().split('\t')
        nclusters = int(fields[0])
        for c in range(nclusters):
            fields = f.readline().strip().split('\t')
            training_gids.extend([int(g) for g in fields[1:]])
    training_gids = set(training_gids)

    # collect metrics
    aps = []
    aucs = []
    accuracies = []
    precisions = []
    with open(metrics_file, 'r') as f:
        for i in range(14):
            f.readline()
        iter = 0
        while (True):
            line = f.readline()
            if line == '':
                break

            anomaly_scores = [float(x) for x in line.strip().split(' ')]
            cluster_map = [int(x) for x in f.readline().strip().split(' ')]
            ngraphs = len(anomaly_scores)

            if iter % spacing != 0:
                iter += 1
                continue

            iter += 1

            y_true = []
            y_score = []
            anomaly_seen = False
            accuracy = []
            precision = []
            for gid in range(ngraphs):
                if gid in training_gids:
                    continue

                if cluster_map[gid] == UNSEEN:
                    continue
                
                # get true label
                if gid >= 300 and gid < 400:
                    y_true.append(1) # anomaly
                    anomaly_seen = True
                    if cluster_map[gid] == ANOMALY:
                        # tp
                        accuracy.append(1)
                        precision.append(1)
                    else:
                        # fp
                        accuracy.append(0)
                        precision.append(0)
                else:
                    y_true.append(0) # not anomaly
                    if cluster_map[gid] != ANOMALY:
                        # tn
                        accuracy.append(1)
                    else:
                        # fn
                        accuracy.append(0)

                # get prediction (anomaly) score
                y_score.append(anomaly_scores[gid])

            if not anomaly_seen:
                continue

            accuracy = np.sum(accuracy) / float(len(accuracy))
            precision = np.sum(precision) / float(len(precision))
            ap = average_precision_score(y_true, y_score)
            auc = roc_auc_score(y_true, y_score)

            accuracies.append(accuracy)
            precisions.append(precision)
            aps.append(ap)
            aucs.append(auc)

    return aps, aucs, accuracies, precisions

def plot_streaming_metrics(aps, aucs, accuracies, precisions):
    plt.rcParams.update({'axes.titlesize': 'large', 'axes.labelsize': 'large'})
    plt.figure()
    plt.hold(True)
    plt.plot([1 + x for x in range(len(aps))], aps, 'r-', label='AP', alpha=0.7)
    plt.plot([1 + x for x in range(len(aucs))], aucs, 'b-', label='AUC', alpha=0.7)
    plt.plot([1 + x for x in range(len(accuracies))], accuracies, 'g-', label='Accuracy', alpha=0.7)
    #plt.plot([1 + x for x in range(len(precisions))], precisions, 'm-', label='Precision', alpha=0.7)
    plt.ylim([0.0, 1.05])
    plt.xlim([1, len(aps)])
    plt.xlabel('Batch Number')
    plt.ylabel('Metric')
    plt.legend(loc='lower right')
    plt.savefig('streaming_metric_plot.pdf', bbox_inches='tight')

if __name__ == "__main__":
    metrics_file = sys.argv[1]
    bootstrap_file = sys.argv[2]
    spacing = 1
    if len(sys.argv) > 3:
        spacing = int(sys.argv[3])
    aps, aucs, accuracies, precisions = compute_streaming_metrics(metrics_file, bootstrap_file, spacing)
    plot_streaming_metrics(aps, aucs, accuracies, precisions)
