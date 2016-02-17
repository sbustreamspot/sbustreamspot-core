/* 
 * Copyright 2016 Emaad Ahmed Manzoor
 * License: Apache License, Version 2.0
 */

#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <deque>
#include <iostream>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cluster.h"
#include "graph.h"
#include "hash.h"
#include "io.h"
#include "param.h"
#include "simhash.h"
#include "streamhash.h"

using namespace std;

void allocate_random_bits(vector<vector<uint64_t>>&, mt19937_64&, uint32_t);
void compute_similarities(const vector<shingle_vector>& shingle_vectors,
                          const vector<bitset<L>>& simhash_sketches,
                          const vector<bitset<L>>& streamhash_sketches);
void construct_random_vectors(vector<vector<int>>& random_vectors,
                              uint32_t rvsize,
                              bernoulli_distribution& bernoulli,
                              mt19937_64& prng);
void construct_simhash_sketches(const vector<shingle_vector>& shingle_vectors,
                                const vector<vector<int>>& random_vectors,
                                vector<bitset<L>>& simhash_sketches);
void perform_lsh_banding(const vector<uint32_t>& normal_gids,
                         const vector<bitset<L>>& simhash_sketches,
                         vector<unordered_map<bitset<R>,vector<uint32_t>>>&
                            hash_tables);
void print_lsh_clusters(const vector<uint32_t>& normal_gids,
                        const vector<bitset<L>>& simhash_sketches,
                        const vector<unordered_map<bitset<R>,vector<uint32_t>>>&
                            hash_tables);
void test_anomalies(uint32_t num_graphs,
                    const vector<bitset<L>>& simhash_sketches,
                    const vector<unordered_map<bitset<R>,vector<uint32_t>>>&
                      hash_tables);

void print_usage() {
  cout << "USAGE: ./swoosh <edge file> <chunk length> <bootstrap file> <parallelism>";
  cout << " <max frac edges> <scenario>" << endl;
}

int main(int argc, char *argv[]) {
  vector<vector<uint64_t>> H(L);                 // Universal family H, contains
                                                 // L hash functions, each
                                                 // represented by chunk_length+2
                                                 // 64-bit random integers

  mt19937_64 prng(SEED);                         // Mersenne Twister 64-bit PRNG
  bernoulli_distribution bernoulli(0.5);         // to generate random vectors
  vector<vector<int>> random_vectors(L);         // |S|-element random vectors
  unordered_map<string,uint32_t> shingle_id;
  unordered_set<string> unique_shingles;
  //vector<unordered_map<bitset<R>,vector<uint32_t>>> hash_tables(B);

  // for timing
  chrono::time_point<chrono::steady_clock> start;
  chrono::time_point<chrono::steady_clock> end;
  chrono::nanoseconds diff;

  // arguments
  if (argc != 7) {
    print_usage();
    return -1;
  }

  string edge_file(argv[1]);
  uint32_t chunk_length = atoi(argv[2]);
  string bootstrap_file(argv[3]);
  uint32_t par = atoi(argv[4]);
  uint32_t max_num_edges = atoi(argv[5]);
  string dataset(argv[6]);

  unordered_set<uint32_t> scenarios;
  if (dataset.compare("dcf") == 0) {
    scenarios.insert(1);
    scenarios.insert(2);
    scenarios.insert(5);
    scenarios.insert(3); // attack
  } else if (dataset.compare("ygf") == 0) {
    scenarios.insert(0);
    scenarios.insert(4);
    scenarios.insert(5);
    scenarios.insert(3); // attack
  } else {
    scenarios.insert(0);
    scenarios.insert(1);
    scenarios.insert(2);
    scenarios.insert(3);
    scenarios.insert(4);
    scenarios.insert(5);
  }

  // FIXME: Tailored for this configuration now
  assert(K == 1 && chunk_length >= 4);

  // read bootstrap clusters and thresholds
  vector<vector<uint32_t>> clusters;
  vector<double> cluster_thresholds;
  vector<int> cluster_map(600, UNSEEN); // gid -> cluster id
  double global_threshold;

  tie(clusters, cluster_thresholds, global_threshold) =
    read_bootstrap_clusters(bootstrap_file);
  unordered_set<uint32_t> train_gids;
  uint32_t nclusters = clusters.size();
  vector<uint32_t> cluster_sizes(nclusters);
  for (uint32_t i = 0; i < nclusters; i++) {
    cluster_sizes[i] = clusters[i].size();
    for (auto& gid : clusters[i]) {
      train_gids.insert(gid);
      cluster_map[gid] = i;
    }
  }

  cout << "Executing with chunk length: " << chunk_length;
  cout << " L: " << L;
  cout << " max edges: " << max_num_edges;
  cout << " par: " << par;
  cout << " data: " << dataset << endl;

  uint32_t num_graphs;
  vector<edge> train_edges;
  unordered_map<uint32_t,vector<edge>> test_edges;
  uint32_t num_test_edges;
  tie(num_graphs, train_edges, test_edges, num_test_edges) =
    read_edges(edge_file, train_gids, scenarios);

#ifdef DEBUG
  for (auto& e : train_edges) {
    assert(train_gids.find(get<5>(e)) != train_gids.end());
  }
#endif

  // make groups of size par (parallel flowing graphs)
  vector<uint32_t> test_gids;
  for (uint32_t i = 0; i < num_graphs; i++) {
    if (scenarios.find(i/100) == scenarios.end()) {
      continue;
    }
    if (train_gids.find(i) == train_gids.end()) {
      test_gids.push_back(i);
    }
  }
  shuffle(test_gids.begin(), test_gids.end(), prng);

  vector<vector<uint32_t>> groups;
  for (uint32_t i = 0; i < test_gids.size(); i += par) {
    vector<uint32_t> group;

    uint32_t end_limit;
    if (test_gids.size() - i < par) {
      end_limit = test_gids.size() - i;
    } else {
      end_limit = par;
    }

    for (uint32_t j = 0; j < end_limit; j++) {
      group.push_back(test_gids[i + j]);
    }

    groups.push_back(group);
  }

#ifdef DEBUG
  for (auto& group : groups) {
    for (auto& g : group) {
      cout << g << " ";
    }
    cout << endl;
  }
#endif

  // per-graph data structures
  vector<graph> graphs(num_graphs);
  vector<bitset<L>> streamhash_sketches(num_graphs);
  vector<vector<int>> streamhash_projections(num_graphs, vector<int>(L, 0));
  vector<bitset<L>> simhash_sketches(num_graphs);
  vector<shingle_vector> shingle_vectors(num_graphs);

  // construct bootstrap graphs offline
  cout << "Constructing " << train_gids.size() << " training graphs:" << endl;
  for (auto& e : train_edges) {
    update_graphs(e, graphs);
  }

  // set up universal hash family for StreamHash
  allocate_random_bits(H, prng, chunk_length);

  // construct StreamHash sketches for bootstrap graphs offline
  cout << "Constructing StreamHash sketches for training graphs:" << endl;
  for (auto& gid : train_gids) {
    unordered_map<string,uint32_t> temp_shingle_vector =
      construct_temp_shingle_vector(graphs[gid], chunk_length);
    tie(streamhash_sketches[gid], streamhash_projections[gid]) =
      construct_streamhash_sketch(temp_shingle_vector, H);
  }

#ifdef DEBUG
  // StreamHash similarity has been verified to be accurate for C=50
  for (auto& gid1 : train_gids) {
    for (auto& gid2 : train_gids) {
      cout << gid1 << "\t" << gid2 << "\t";
      cout << cos(PI*(1.0 - streamhash_similarity(streamhash_sketches[gid1],
                                                  streamhash_sketches[gid2])));
      cout << endl;
    }
  }
#endif

  // per-cluster data structures
  vector<vector<double>> centroid_projections;
  vector<bitset<L>> centroid_sketches;

  // construct cluster centroid sketches/projections
  cout << "Constructing bootstrap cluster centroids:" << endl;
  tie(centroid_sketches, centroid_projections) =
    construct_centroid_sketches(streamhash_projections, clusters, nclusters);

  // compute distances of training graphs to their cluster centroids
  vector<double> anomaly_scores(num_graphs, UNSEEN);
  for (auto& gid : train_gids) {
    // anomaly score is a "distance", hence the 1.0 - x
    anomaly_scores[gid] = 1.0 -
      cos(PI*(1.0 - streamhash_similarity(streamhash_sketches[gid],
                                          centroid_sketches[cluster_map[gid]])));
  }

#ifdef DEBUG
  for (auto& s : anomaly_scores) {
    cout << s << " ";
  }
  cout << endl;
  for (auto& s : cluster_map) {
    cout << s << " ";
  }
  cout << endl;
#endif

  // add test edges to graphs
  cout << "Streaming in " << num_test_edges << " test edges:" << endl;
  vector<chrono::nanoseconds> graph_update_times(num_test_edges,
                                                  chrono::nanoseconds(0));
  vector<chrono::nanoseconds> sketch_update_times(num_test_edges,
                                                   chrono::nanoseconds(0));
  vector<chrono::nanoseconds> shingle_construction_times(num_test_edges,
                                                          chrono::nanoseconds(0));
  vector<chrono::nanoseconds> cluster_update_times(num_test_edges,
                                                    chrono::nanoseconds(0));
  uint32_t num_intervals = ceil(static_cast<double>(num_test_edges) /
                                CLUSTER_UPDATE_INTERVAL);
  if (num_intervals == 0)
    num_intervals = 1; // if interval length is too long
  vector<vector<double>> anomaly_score_iterations(num_intervals,
                                                  vector<double>(num_graphs));
  vector<vector<int>> cluster_map_iterations(num_intervals,
                                             vector<int>(num_graphs));

  uint32_t cache_size = max_num_edges;
  deque<edge> cache;

  uint32_t edge_num = 0;
  for (auto& group : groups) {

#ifdef DEBUG
    cout << "Streaming group: ";
    for (auto& g : group)
      cout << g << " ";
    cout << endl;
#endif

    unordered_map<uint32_t,uint32_t> edge_offset;
    for (auto &g : group)
      edge_offset[g] = 0;

    vector<uint32_t> group_copy(group);
    while (group_copy.size() > 0) {
      // used to pick a random graph
      uniform_int_distribution<uint32_t> rand_group_idx(0,
                                                        group_copy.size() - 1);

      uint32_t gidx = rand_group_idx(prng);
      uint32_t gid = group_copy[gidx];
      uint32_t off = edge_offset[gid];

#ifdef DEBUG
      cout << "\tStreaming graph " << gid << " offset " << off << endl;
#endif

      auto& e = test_edges[gid][off];

      //
      // PROCESS EDGE
      //

      // check if cache is full
      if (cache.size() == cache_size) {
        auto& edge_to_evict = cache.front(); // oldest edge at head
        remove_from_graph(edge_to_evict, graphs);
        cache.pop_front();
      }
      cache.push_back(e); // newest edge at tail

      // update graph
      start = chrono::steady_clock::now();
      update_graphs(e, graphs);
      end = chrono::steady_clock::now();
      diff = chrono::duration_cast<chrono::nanoseconds>(end - start);
      graph_update_times[edge_num] = diff;

      // update sketches
      chrono::nanoseconds shingle_construction_time;
      chrono::nanoseconds sketch_update_time;
      vector<int> projection_delta;
      tie(projection_delta, shingle_construction_time, sketch_update_time) =
        update_streamhash_sketches(e, graphs, streamhash_sketches,
                                   streamhash_projections, chunk_length, H);
      sketch_update_times[edge_num] = sketch_update_time;
      shingle_construction_times[edge_num] = shingle_construction_time;

      // update centroids and centroid-graph distances
      start = chrono::steady_clock::now();
      update_distances_and_clusters(gid, projection_delta,
                                    streamhash_sketches,
                                    streamhash_projections,
                                    centroid_sketches, centroid_projections,
                                    cluster_sizes, cluster_map,
                                    anomaly_scores, global_threshold,
                                    cluster_thresholds);
      end = chrono::steady_clock::now();
      diff = chrono::duration_cast<chrono::nanoseconds>(end - start);
      cluster_update_times[edge_num] = diff;

      // store current anomaly scores and cluster assignments
      if (edge_num % CLUSTER_UPDATE_INTERVAL == 0 ||
          edge_num == num_test_edges - 1) {
        anomaly_score_iterations[edge_num/CLUSTER_UPDATE_INTERVAL] = anomaly_scores;
        cluster_map_iterations[edge_num/CLUSTER_UPDATE_INTERVAL] = cluster_map;
      }

      edge_num++;

#ifdef DEBUG
      chrono::nanoseconds last_graph_update_time =
        graph_update_times[graph_update_times.size() - 1];
      chrono::nanoseconds last_sketch_update_time =
        sketch_update_times[sketch_update_times.size() - 1];
      chrono::nanoseconds last_cluster_update_time =
        cluster_update_times[cluster_update_times.size() - 1];
      cout << "\tMost recent run times: ";
      cout << static_cast<double>(last_graph_update_time.count()) << "us";
      cout << " (graph), ";
      cout << static_cast<double>(last_sketch_update_time.count()) << "us";
      cout << " (sketch), ";
      cout << static_cast<double>(last_cluster_update_time.count()) << "us";
      cout << " (cluster)" << endl;
#endif

      edge_offset[gid]++; // increment next edge offset
      if (edge_offset[gid] == test_edges[gid].size()) {
        // out of edges for this gid
        group_copy.erase(group_copy.begin() + gidx);
#ifdef DEBUG
        cout << "Erasing graph " << group[gidx] << endl;
        cout << "New group: ";
        for (auto& g : group_copy)
          cout << g << " ";
        cout << endl;
#endif
      }
    }
  }

  chrono::nanoseconds mean_graph_update_time(0);
  for (auto& t : graph_update_times) {
    mean_graph_update_time += t;
  }
  mean_graph_update_time /= graph_update_times.size();

  chrono::nanoseconds mean_sketch_update_time(0);
  for (auto& t : sketch_update_times) {
    mean_sketch_update_time += t;
  }
  mean_sketch_update_time /= sketch_update_times.size();

  chrono::nanoseconds mean_shingle_construction_time(0);
  for (auto& t : shingle_construction_times) {
    mean_shingle_construction_time += t;
  }
  mean_shingle_construction_time /= shingle_construction_times.size();

  chrono::nanoseconds mean_cluster_update_time(0);
  for (auto& t : cluster_update_times) {
    mean_cluster_update_time += t;
  }
  mean_cluster_update_time /= cluster_update_times.size();

  cout << "Runtimes (per-edge):" << endl;
  cout << "\tGraph update: ";
  cout << static_cast<double>(mean_graph_update_time.count()) << "us" << endl;
  cout << "\tShingle construction: ";
  cout << static_cast<double>(mean_shingle_construction_time.count()) << "us" << endl;
  cout << "\tSketch update: ";
  cout << static_cast<double>(mean_sketch_update_time.count()) << "us" << endl;
  cout << "\tCluster update: ";
  cout << static_cast<double>(mean_cluster_update_time.count()) << "us" << endl;

  // print size of each test graph in memory
  cout << "Test graph sizes: " << endl;
  for (auto& gid : test_gids) {
    uint32_t size = 0;
    for (auto& kv : graphs[gid]) {
      size += kv.second.size();
    }
    cout << gid << "," << size << " ";
  }
  cout << endl;

  cout << "Iterations " << num_intervals << endl;
  for (uint32_t i = 0; i < num_intervals; i++) {
    auto& a = anomaly_score_iterations[i];
    auto& c = cluster_map_iterations[i];
    for (uint32_t j = 0; j < a.size(); j++) {
      cout << a[j] << " ";
    }
    cout << endl;
    for (uint32_t j = 0; j < a.size(); j++) {
      cout << c[j] << " ";
    }
    cout << endl;
  }

#ifdef DEBUG
  for (uint32_t i = 0; i < num_graphs; i++) {
    cout << "Graph projection " << i << ": ";
    for (uint32_t j = 0; j < 10; j++) {
      cout << streamhash_projections[i][j] << " ";
    }
    cout << endl;
  }

  for (uint32_t i = 0; i < centroid_projections.size(); i++) {
    cout << "Centroid projection " << i << ": ";
    for (uint32_t j = 0; j < 10; j++) {
      cout << centroid_projections[i][j] << " ";
    }
    cout << endl;
  }
#endif

  /*cout << "Constructing shingle vectors:" << endl;
  start = chrono::steady_clock::now();
  construct_shingle_vectors(shingle_vectors, shingle_id, graphs,
                            chunk_length);
  end = chrono::steady_clock::now();
  diff = chrono::duration_cast<chrono::nanoseconds>(end - start);
  cout << "\tShingle vector construction (per-graph): ";
  cout << static_cast<double>(diff.count())/num_graphs << "us" << endl;

  cout << "Constructing Simhash sketches:" << endl;
  construct_random_vectors(random_vectors, shingle_vectors[0].size(),
                           bernoulli, prng);
  construct_simhash_sketches(shingle_vectors, random_vectors,
                             simhash_sketches);

  cout << "Computing pairwise similarities:" << endl;
  compute_similarities(shingle_vectors, simhash_sketches, streamhash_sketches);

  // label attack/normal graph id's
  vector<uint32_t> normal_gids;
  vector<uint32_t> attack_gids;
  if (num_graphs == 600) { // UIC data hack
    for (uint32_t i = 0; i < 300; i++) {
      normal_gids.push_back(i);
    }
    for (uint32_t i = 300; i < 400; i++) {
      attack_gids.push_back(i);
    }
    for (uint32_t i = 400; i < 600; i++) {
      normal_gids.push_back(i);
    }
  } else {
    for (uint32_t i = 0; i < num_graphs/2; i++) {
      normal_gids.push_back(i);
    }
    for (uint32_t i = num_graphs/2; i < num_graphs; i++) {
      attack_gids.push_back(i);
    }
  }

  cout << "Performing LSH banding:" << endl;
  perform_lsh_banding(normal_gids, simhash_sketches, hash_tables);

  cout << "Printing LSH clusters:" << endl;
  print_lsh_clusters(normal_gids, simhash_sketches, hash_tables);

  cout << "Testing anomalies:" << endl;
  test_anomalies(num_graphs, simhash_sketches, hash_tables);*/

  return 0;
}

void allocate_random_bits(vector<vector<uint64_t>>& H, mt19937_64& prng,
                          uint32_t chunk_length) {
  // allocate random bits for hashing
  for (uint32_t i = 0; i < L; i++) {
    // hash function h_i \in H
    H[i] = vector<uint64_t>(chunk_length + 2);
    for (uint32_t j = 0; j < chunk_length + 2; j++) {
      // random number m_j of h_i
      H[i][j] = prng();
    }
  }
#ifdef DEBUG
    cout << "64-bit random numbers:\n";
    for (int i = 0; i < L; i++) {
      for (int j = 0; j < chunk_length + 2; j++) {
        cout << H[i][j] << " ";
      }
      cout << endl;
    }
#endif
}

void compute_similarities(const vector<shingle_vector>& shingle_vectors,
                          const vector<bitset<L>>& simhash_sketches,
                          const vector<bitset<L>>& streamhash_sketches) {
  for (uint32_t i = 0; i < shingle_vectors.size(); i++) {
    for (uint32_t j = 0; j < shingle_vectors.size(); j++) {
      double cosine = cosine_similarity(shingle_vectors[i],
                                        shingle_vectors[j]);
      double angsim = 1 - acos(cosine)/PI;
      double simhash_sim = simhash_similarity(simhash_sketches[i],
                                              simhash_sketches[j]);
      double streamhash_sim = streamhash_similarity(streamhash_sketches[i],
                                                    streamhash_sketches[j]);
      cout << i << "\t" << j << "\t";
      cout << cosine;
      cout << "\t" << angsim;
      cout << "\t" << simhash_sim << "," << cos(PI*(1.0 - simhash_sim)) << " ";
      cout << "\t" << streamhash_sim << "," << cos(PI*(1.0 - streamhash_sim)) << " ";
      cout << "\t" << (streamhash_sim - angsim);
      cout << endl;
    }
  }
}

void construct_random_vectors(vector<vector<int>>& random_vectors,
                              uint32_t rvsize,
                              bernoulli_distribution& bernoulli,
                              mt19937_64& prng) {
  // allocate L |S|-element {+1,-1} random vectors
  for (uint32_t i = 0; i < L; i++) {
    random_vectors[i].resize(rvsize);
    for (uint32_t j = 0; j < rvsize; j++) {
      random_vectors[i][j] = 2 * static_cast<int>(bernoulli(prng)) - 1;
    }
  }

#ifdef VERBOSE 
  cout << "Random vectors:\n";
  for (uint32_t i = 0; i < L; i++) {
    cout << "\t";
    for (int rv_i : random_vectors[i]) {
      cout << rv_i << " ";
    }
    cout << endl;
  }
#endif
}

void construct_simhash_sketches(const vector<shingle_vector>& shingle_vectors,
                                const vector<vector<int>>& random_vectors,
                                vector<bitset<L>>& simhash_sketches) {
  // compute SimHash sketches
  for (uint32_t i = 0; i < simhash_sketches.size(); i++) {
    construct_simhash_sketch(simhash_sketches[i], shingle_vectors[i],
                             random_vectors);
  }

#ifdef DEBUG
  cout << "SimHash sketches:\n";
  for (uint32_t i = 0; i < simhash_sketches.size(); i++) {
    cout << "\t" << simhash_sketches[i].to_string() << endl;
  }
#endif
}

void perform_lsh_banding(const vector<uint32_t>& normal_gids,
                         const vector<bitset<L>>& simhash_sketches,
                         vector<unordered_map<bitset<R>,vector<uint32_t>>>&
                            hash_tables) {
  // LSH-banding: assign graphs to hashtable buckets
  for (auto& gid : normal_gids) {
    hash_bands(gid, simhash_sketches[gid], hash_tables);
  }
#ifdef DEBUG
  cout << "Hash tables after hashing bands:\n";
  for (uint32_t i = 0; i < B; i++) {
    cout << "\tHash table " << i << ":\n";
    for (auto& kv : hash_tables[i]) {
      // print graph id's in this bucket
      cout << "\t\tBucket => ";
      for (uint32_t j = 0; j < kv.second.size(); j++) {
        cout << kv.second[j] << " ";
      }
      cout << endl;
    }
  }
#endif
}

void print_lsh_clusters(const vector<uint32_t>& normal_gids,
                        const vector<bitset<L>>& simhash_sketches,
                        const vector<unordered_map<bitset<R>,vector<uint32_t>>>&
                            hash_tables) {
  unordered_set<uint32_t> graphs(normal_gids.size());
  for (auto& gid : normal_gids) {
    graphs.insert(gid);
  }

  while (!graphs.empty()) {
    uint32_t gid = *(graphs.begin());
    unordered_set<uint32_t> cluster;

    queue<uint32_t> q;
    q.push(gid);
    while (!q.empty()) {
      uint32_t g = q.front();
      q.pop();

      cluster.insert(g);

      unordered_set<uint32_t> shared_bucket_graphs;
      get_shared_bucket_graphs(simhash_sketches[g], hash_tables,
                               shared_bucket_graphs);

#ifdef DEBUG
      cout << "\tGraphs sharing buckets with: " << g << " => ";
      for (auto& e : shared_bucket_graphs) {
       cout << e << " ";
      }
      cout << endl;
#endif

      for (auto& h : shared_bucket_graphs) {
        if (cluster.find(h) == cluster.end()) {
          q.push(h);
        }
      }
    }

    for (auto& e : cluster) {
      cout << e << " ";
    }
    cout << endl;

    for (auto& e : cluster) {
      graphs.erase(e);
    }
  }
}

void test_anomalies(uint32_t num_graphs,
                    const vector<bitset<L>>& simhash_sketches,
                    const vector<unordered_map<bitset<R>,vector<uint32_t>>>&
                      hash_tables) {
  // for each attack graph, hash it to the B hash tables
  // if any bucket hashed to contains a graph, the attack is not an anomaly
  // otherwise, the graph is isolated, and is an anomaly
  for (uint32_t gid = 0; gid < num_graphs; gid++) {
    cout << gid << "\t";
    if (is_isolated(simhash_sketches[gid], hash_tables)) {
      cout << "T" << endl;
    } else {
      cout << "F" << endl;
    }
  }
}
