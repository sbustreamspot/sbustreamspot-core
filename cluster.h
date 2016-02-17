/* 
 * Copyright 2016 Emaad Ahmed Manzoor
 * License: Apache License, Version 2.0
 * http://www3.cs.stonybrook.edu/~emanzoor/streamspot/
 */

#ifndef STREAMSPOT_CLUSTER_H_
#define STREAMSPOT_CLUSTER_H_

#include <bitset>
#include "cluster.h"
#include "param.h"
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define ANOMALY -1  // cluster ID for anomaly
#define UNSEEN -2   // cluster ID for unseen graphs

namespace std {

void hash_bands(uint32_t gid, const bitset<L>& sketch,
                vector<unordered_map<bitset<R>,vector<uint32_t>>>& hash_tables);
bool is_isolated(const bitset<L>& sketch,
                 const vector<unordered_map<bitset<R>,vector<uint32_t>>>&
                    hash_tables);
void get_shared_bucket_graphs(const bitset<L>& sketch,
                              const vector<unordered_map<bitset<R>,
                                                   vector<uint32_t>>>& hash_tables,
                              unordered_set<uint32_t>& shared_bucket_graphs);
tuple<vector<bitset<L>>, vector<vector<double>>>
construct_centroid_sketches(const vector<vector<int>>& streamhash_projections,
                            const vector<vector<uint32_t>>& bootstrap_clusters,
                            uint32_t nclusters);
void update_distances_and_clusters(uint32_t gid,
                                   const vector<int>& projection_delta,
                                   const vector<bitset<L>>& graph_sketches,
                                   const vector<vector<int>>& graph_projections,
                                   vector<bitset<L>>& centroid_sketches,
                                   vector<vector<double>>& centroid_projections,
                                   vector<uint32_t>& cluster_sizes,
                                   vector<int>& cluster_map,
                                   vector<double>& anomaly_scores,
                                   double anomaly_threshold,
                                   const vector<double>& cluster_thresholds);

}

#endif
