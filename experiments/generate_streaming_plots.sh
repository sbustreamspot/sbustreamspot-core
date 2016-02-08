#!/usr/bin/env bash

for C in 25 50
do
  for par in 10 20 30 40 50 100
  do
    for L in 10 100 500 1000 1500 2000
    do
      filename=anomaly_scores_C${C}_k10_pct75_thresh10_par${par}_L${L}_nooutliers.txt
      bootstrap=bootstrap_clusters_C${C}_k10_pct75_nooutliers.txt
      out=streaming_metrics_C${C}_k10_pct75_thresh10_par${par}_L${L}_fine.pdf
      if [ -e $filename ]
      then
        echo "$C $par $L"
        python plot_streaming_metrics.py $filename $bootstrap
        mv streaming_metric_plot.pdf $out 
      fi
      
      filename=anomaly_scores_C${C}_k10_pct75_thresh10_par${par}_L${L}.txt
      bootstrap=bootstrap_clusters_C${C}_k10_pct75.txt
      out=streaming_metrics_C${C}_k10_pct75_thresh10_par${par}_L${L}_coarse.pdf
      if [ -e $filename ]
      then
        echo "$C $par $L"
        python plot_streaming_metrics.py $filename $bootstrap
        mv streaming_metric_plot.pdf $out 
      fi
    done
  done
done
