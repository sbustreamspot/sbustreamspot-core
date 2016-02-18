# StreamSpot

This repository contains the core streaming heterogenous graph clustering
and anomaly detection code.

Before attempting execution, ensure you have the following available as two
separate files:

   * Edges: A file containing one edge per line for all input graphs in the
     dataset. A sample is provided as `test_edges.txt`. The edge file used
     for the experiments in the paper is available at [sbustreamspot-data][1].

   * Bootstrap clusters: A file describing the bootstrap clusters. A
     sample is provided as `test_bootstrap_clusters.txt`. Bootstrap clusters
     used for the experiments in the paper are available [here][2].

Compilation and execution has been tested with GCC 5.2.1 on Ubuntu 15.10.

## Quickstart

```
git clone https://github.com/sbustreamspot/sbustreamspot-core.git
cd sbustreamspot-core
make clean optimized
./streamspot --edges=test_edges.txt \
             --bootstrap=test_bootstrap_clusters.txt \
             --chunk-length=10 \
             --num-parallel-graphs=10 \
             --max-num-edges=10 \
             --dataset=all
```

To run with a different dataset, change `test_edges.txt` and
`test_bootstrap_clusters.txt` accordingly.

## Parameters

Most parameter settings are via the command-line; details on setting
them can be viewed by running `./streamspot --help`.

A few parameters are set at compile-time and can be found in `param.h`.

## Contact

   * emanzoor@cs.stonybrook.edu
   * leman@cs.stonybrook.edu

[1]: https://github.com/sbustreamspot/sbustreamspot-data
[2]: https://gist.github.com/emaadmanzoor/118846a642727a0bf704
