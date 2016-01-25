# Evaluate Chunked Shingling for Clustering

This experiment evaluates the influence of chunked shingling on graph clustering.

## Quickstart

   * Run the experiment: `python chunked_shingling_evaluation.py`
   * k-medoids code is from [here](https://github.com/alexprengere/medoids).

## Procedure

   * For different values of chunk length `C`, compute the pairwise similarity
     between all 600 graphs in the UIC data.
   * Run k-medoids clustering (with `k=6`) with the pairwise similarities as
     input.
   * Measure the cluster purity for each value of `C`.

Note that k-medoids is initialized randomly, so YMMV. The provided code
seeds the random number generator so that results are consistent across runs.

## Results

   * Having a large chunk length is equivalent to non-chunked clustering.
   * Having smaller chunk lengths does seem to improve cluster purity.
   * The influence is not predictable (yet); we can see the cluster purity
     both increases and decreases as `C` is increased. 

Note that labels 0 - 4 are the first 5 UIC scenarios, and label 5 is the
attack scenario. The end-goal is to isolate this scenario.

```
Clusters for C: 25
  Cluster label frequencies: Counter({4: 98, 5: 20})
  Cluster label frequencies: Counter({5: 78})
  Cluster label frequencies: Counter({0: 100, 2: 100, 1: 2, 4: 2, 5: 2})
  Cluster label frequencies: Counter({3: 100})
  Cluster label frequencies: Counter({1: 49})
  Cluster label frequencies: Counter({1: 49})
  Cluster purity: 0.79

Clusters for C: 50
  Cluster label frequencies: Counter({3: 100})
  Cluster label frequencies: Counter({1: 98})
  Cluster label frequencies: Counter({0: 98, 2: 29, 5: 2})
  Cluster label frequencies: Counter({4: 98, 5: 19})
  Cluster label frequencies: Counter({5: 78})
  Cluster label frequencies: Counter({2: 71, 0: 2, 1: 2, 4: 2, 5: 1})
  Cluster purity: 0.905

Clusters for C: 100
  Cluster label frequencies: Counter({3: 100})
  Cluster label frequencies: Counter({4: 52, 5: 9})
  Cluster label frequencies: Counter({4: 46, 5: 11})
  Cluster label frequencies: Counter({0: 100, 2: 100, 5: 80, 1: 2})
  Cluster label frequencies: Counter({1: 48, 4: 2})
  Cluster label frequencies: Counter({1: 50})
  Cluster purity: 0.66

Clusters for C: 150
  Cluster label frequencies: Counter({3: 100})
  Cluster label frequencies: Counter({1: 98, 4: 2})
  Cluster label frequencies: Counter({0: 100, 2: 100, 5: 19, 1: 2})
  Cluster label frequencies: Counter({4: 63, 5: 3})
  Cluster label frequencies: Counter({4: 35, 5: 17})
  Cluster label frequencies: Counter({5: 61})
  Cluster purity: 0.761666666667

Clusters for C: 200
  Cluster label frequencies: Counter({3: 100})
  Cluster label frequencies: Counter({4: 98, 5: 20})
  Cluster label frequencies: Counter({1: 98, 4: 2, 5: 1})
  Cluster label frequencies: Counter({5: 61, 2: 4})
  Cluster label frequencies: Counter({0: 13})
  Cluster label frequencies: Counter({2: 96, 0: 87, 5: 18, 1: 2})
  Cluster purity: 0.776666666667

Clusters for C: 300
  Cluster label frequencies: Counter({1: 98, 2: 1, 4: 1})
  Cluster label frequencies: Counter({2: 99, 0: 97, 5: 10, 1: 2, 4: 1})
  Cluster label frequencies: Counter({5: 4})
  Cluster label frequencies: Counter({4: 98, 5: 15})
  Cluster label frequencies: Counter({5: 71})
  Cluster label frequencies: Counter({3: 100, 0: 3})
  Cluster purity: 0.783333333333

Clusters for C: 500
  Cluster label frequencies: Counter({2: 99, 0: 46, 1: 18, 5: 4})
  Cluster label frequencies: Counter({4: 97, 5: 12})
  Cluster label frequencies: Counter({3: 64, 1: 57, 0: 40, 4: 2, 2: 1})
  Cluster label frequencies: Counter({4: 1})
  Cluster label frequencies: Counter({3: 36, 1: 25, 0: 14})
  Cluster label frequencies: Counter({5: 84})
  Cluster purity: 0.635
```
