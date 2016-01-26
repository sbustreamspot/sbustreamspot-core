#!/usr/bin/env bash

for pct in 1.0 0.8 0.6 0.4 0.2
do
  echo "pct_${pct/./_}.txt"
  python precision_recall_plot.py $pct > "pct_${pct/./_}.txt" &
done
