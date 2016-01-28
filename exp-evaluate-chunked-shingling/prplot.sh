#!/usr/bin/env bash

for k in 4 5 6 7 8 9
do
  python precision_recall_plot.py $k > "roc_pr_k$k.txt" &
done
