/* 
 * Copyright 2016 Emaad Ahmed Manzoor
 * License: Apache License, Version 2.0
 * http://www3.cs.stonybrook.edu/~emanzoor/streamspot/
 */

#include <algorithm>
#include <bitset>
#include <cassert>
#include "cluster.h"
#include <cmath>
#include <iostream>
#include "param.h"
#include <string>
#include "streamhash.h"
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace std {

void hash_bands(uint32_t gid, const bitset<L>& sketch,
                vector<unordered_map<bitset<R>,vector<uint32_t>>>& hash_tables) {
#ifdef DEBUG
  cout << "Hashing bands of GID: " << gid << endl;
#endif

  bitset<L> mask = bitset<L>(string(R, '1')); // R one's
  for (uint32_t i = 0; i < B; i++) {
    // get the i'th R-bit band
    string band_string = (sketch >> (R * i) & mask).to_string();
    band_string = band_string.substr(band_string.length() - R, R);
    bitset<R> band(band_string);
#ifdef DEBUG
    cout << "\tBand " << i << ": " << band.to_string() << endl;
#endif

    // hash the band to a bucket in the i'th hash table and insert the gid
    hash_tables[i][band].push_back(gid);
  }
}

bool is_isolated(const bitset<L>& sketch,
                 const vector<unordered_map<bitset<R>,vector<uint32_t>>>&
                    hash_tables) {
  bitset<L> mask = bitset<L>(string(R, '1')); // R one's
  for (uint32_t i = 0; i < B; i++) {
    string band_string = (sketch >> (R * i) & mask).to_string();
    band_string = band_string.substr(band_string.length() - R, R);
    bitset<R> band(band_string);
    if (hash_tables[i].find(band) != hash_tables[i].end()) {
      return false;
    }
  }
  return true;
}

void get_shared_bucket_graphs(const bitset<L>& sketch,
                              const vector<unordered_map<bitset<R>,
                                           vector<uint32_t>>>& hash_tables,
                              unordered_set<uint32_t>& shared_bucket_graphs) {
  bitset<L> mask = bitset<L>(string(R, '1')); // R one's
  for (uint32_t i = 0; i < B; i++) {
    // get the i'th R-bit band
    string band_string = (sketch >> (R * i) & mask).to_string();
    band_string = band_string.substr(band_string.length() - R, R);
    bitset<R> band(band_string);

    for (auto& gid : hash_tables[i].at(band)) {
      shared_bucket_graphs.insert(gid);
    }
  }
}

tuple<vector<bitset<L>>, vector<vector<double>>>
construct_centroid_sketches(const vector<vector<int>>& streamhash_projections,
                            const vector<vector<uint32_t>>& clusters,
                            uint32_t nclusters) {
  vector<bitset<L>> centroid_sketches(nclusters);
  vector<vector<double>> centroid_projections(nclusters, vector<double>(L, 0.0));

  for (uint32_t c = 0; c < nclusters; c++) {
    for (auto& gid : clusters[c]) {
      // add the projection vector of this graph to the centroid's
      for (uint32_t l = 0; l < L; l++) {
        centroid_projections[c][l] += streamhash_projections[gid][l];
      }
    }
  }

  // now the centroid projections contain the sum of all projections of their cluster
  for (uint32_t c = 0; c < nclusters; c++) {
    for (uint32_t l = 0; l < L; l++) {
      centroid_projections[c][l] /= clusters[c].size();
      centroid_sketches[c][l] = centroid_projections[c][l] >= 0 ? 1 : 0;
    }
  }

  return make_tuple(centroid_sketches, centroid_projections);
}

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
                                   const vector<double>& cluster_thresholds) {
  // calculate distance of graph to all cluster centroids
  uint32_t nclusters = cluster_sizes.size();
  vector<double> distances(nclusters);
  double min_distance = 5.0;
  int nearest_cluster = -1;
#ifdef DEBUG
  cout << "\tUpdating edge for gid: " << gid << endl;
  cout << "\tDistances: ";
#endif
  for (uint32_t i = 0; i < nclusters; i++) {
    distances[i] = 1.0 -
      cos(PI*(1.0 - streamhash_similarity(graph_sketches[gid],
                                          centroid_sketches[i])));
#ifdef DEBUG
    cout << distances[i] << " ";
#endif
    if (distances[i] < min_distance) {
      min_distance = distances[i];
      nearest_cluster = i;
    }
  }
#ifdef DEBUG
  cout << endl;
#endif

  // set its anomaly score to distance from nearest centroid
  anomaly_scores[gid] = min_distance;
  int current_cluster = cluster_map[gid];
#ifdef DEBUG
  cout << "\tCurrent cluster: " << current_cluster << endl;
#endif

  // if distance > threshold: outlier
  if (min_distance > min(anomaly_threshold,
                         cluster_thresholds[nearest_cluster])) {
    // change cluster mapping to ANOMALY
    cluster_map[gid] = ANOMALY;

    // if part of a cluster currently
    if (current_cluster != UNSEEN && current_cluster != ANOMALY) {
      // remove from cluster
      int old_cluster_size = cluster_sizes[current_cluster];
      cluster_sizes[current_cluster]--;

      // update cluster centroid projection/sketch
      auto& centroid_p = centroid_projections[current_cluster];
      auto& centroid_s = centroid_sketches[current_cluster];
      auto& graph_projection = graph_projections[gid];
      for (uint32_t l = 0; l < L; l++) {
        centroid_p[l] = (centroid_p[l] * old_cluster_size -
                          (graph_projection[l] - projection_delta[l])) /
                        (old_cluster_size - 1);
        centroid_s[l] = centroid_p[l] >= 0 ? 1 : 0;
      }

      // update anomaly score if current cluster == nearest cluster (centroid moved)
      if (current_cluster == nearest_cluster) {
        anomaly_scores[gid] = 1.0 -
          cos(PI*(1.0 - streamhash_similarity(graph_sketches[gid],
                                              centroid_s)));
      }
    }
  } else { // else if distance <= threshold:
    // if current cluster != nearest centroid:
    if (current_cluster != nearest_cluster) {
      // change cluster mapping from current to new cluster
      cluster_map[gid] = nearest_cluster;
#ifdef DEBUG
      cout << "\tNew cluster: " << nearest_cluster << endl;
#endif

      // if a previous cluster existed
      if (current_cluster != UNSEEN && current_cluster != ANOMALY) {
        // remove from current cluster
        int old_cluster_size = cluster_sizes[current_cluster];
        cluster_sizes[current_cluster]--;

        // update cluster centroid projection/sketch
        auto& centroid_p = centroid_projections[current_cluster];
        auto& centroid_s = centroid_sketches[current_cluster];
        auto& graph_projection = graph_projections[gid];

#ifdef DEBUG
        cout << "\tPrev. cluster centroid before removing graph:";
        for (uint32_t j = 0; j < 10; j++)
          cout << centroid_p[j] << " ";
        cout << endl;
#endif

        for (uint32_t l = 0; l < L; l++) {
          centroid_p[l] = (centroid_p[l] * old_cluster_size -
                            (graph_projection[l] - projection_delta[l])) /
                          (old_cluster_size - 1);
          centroid_s[l] = centroid_p[l] >= 0 ? 1 : 0;
        }

#ifdef DEBUG
        cout << "\tPrev. cluster centroid after removing graph:";
        for (uint32_t j = 0; j < 10; j++)
          cout << centroid_p[j] << " ";
        cout << endl;
#endif
        // the old cluster centroid moved, but the nearest cluster did not yet
        // so don't modify the anomaly score yet
      }

      // add to new cluster
      int old_cluster_size = cluster_sizes[nearest_cluster];
      cluster_sizes[nearest_cluster]++;

      // update new cluster centroid projection/sketch
      auto& centroid_p = centroid_projections[nearest_cluster];
      auto& centroid_s = centroid_sketches[nearest_cluster];
      auto& graph_projection = graph_projections[gid];

#ifdef DEBUG
      cout << "\tNew cluster centroid before adding graph: ";
      for (uint32_t j = 0; j < 10; j++)
        cout << centroid_p[j] << " ";
      cout << endl;

      cout << "\tAdding graph: ";
      for (uint32_t j = 0; j < 10; j++)
        cout << graph_projection[j] << " ";
      cout << endl;
#endif

      for (uint32_t l = 0; l < L; l++) {
        centroid_p[l] = (centroid_p[l] * old_cluster_size + graph_projection[l]) /
                        (old_cluster_size + 1);
        centroid_s[l] = centroid_p[l] >= 0 ? 1 : 0;
      }

      // update anomaly score wrt. nearest cluster (centroid moved)
      anomaly_scores[gid] = 1.0 -
        cos(PI*(1.0 - streamhash_similarity(graph_sketches[gid],
                                            centroid_s)));

#ifdef DEBUG
      cout << "\tNew cluster centroid after adding graph: ";
      for (uint32_t j = 0; j < 10; j++)
        cout << centroid_p[j] << " ";
      cout << endl;

      cout << "\tNew anomaly score: " << anomaly_scores[gid] << endl;
#endif
    } else { // current_cluster = nearest_centroid
      // only update the current_cluster centroid using the projection delta
      int current_cluster_size = cluster_sizes[current_cluster];
      auto& centroid_p = centroid_projections[current_cluster];
      auto& centroid_s = centroid_sketches[current_cluster];

#ifdef DEBUG
      cout << "\tModified graph: ";
      for (uint32_t j = 0; j < 10; j++)
        cout << graph_projections[gid][j] << " ";
      cout << endl;
      cout << "\tDelta: ";
      for (uint32_t j = 0; j < 10; j++)
        cout << projection_delta[j] << " ";
      cout << endl;
#endif

      for (uint32_t l = 0; l < L; l++) {
        centroid_p[l] += static_cast<double>(projection_delta[l]) /
                         current_cluster_size;
        centroid_s[l] = centroid_p[l] >= 0 ? 1 : 0;
      }

      // update anomaly score wrt. nearest cluster (centroid moved)
      anomaly_scores[gid] = 1.0 -
        cos(PI*(1.0 - streamhash_similarity(graph_sketches[gid],
                                            centroid_s)));

#ifdef DEBUG
      cout << "\tExisting cluster centroid after modifying graph: ";
      for (uint32_t j = 0; j < 10; j++)
        cout << centroid_p[j] << " ";
      cout << endl;

      cout << "\tNew anomaly score: " << anomaly_scores[gid] << endl;
#endif
    }
  }
}

}
