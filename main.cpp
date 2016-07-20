/* 
 * Copyright 2016 Emaad Ahmed Manzoor
 * License: Apache License, Version 2.0
 * http://www3.cs.stonybrook.edu/~emanzoor/streamspot/
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
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cluster.h"
#include "docopt.h"
#include "graph.h"
#include "hash.h"
#include "io.h"
#include "param.h"
#include "simhash.h"
#include "streamhash.h"

using namespace std;

static const char USAGE[] =
R"(StreamSpot.

    Usage:
      streamspot --edges=<edge file>
                 --bootstrap=<bootstrap clusters file>

      streamspot (-h | --help)

    Options:
      -h, --help                              Show this screen.
      --edges=<edge file>                     Training edges.
      --bootstrap=<bootstrap clusters file>   Bootstrap clusters.
)";

void allocate_random_bits(vector<vector<uint64_t>>&, mt19937_64&, uint32_t);
void compute_similarities(const vector<shingle_vector>& shingle_vectors,
                          const vector<bitset<L>>& simhash_sketches,
                          const vector<bitset<L>>& streamhash_sketches,
                          const unordered_map<uint32_t,int>& cluster_map);
void construct_random_vectors(vector<vector<int>>& random_vectors,
                              uint32_t rvsize,
                              bernoulli_distribution& bernoulli,
                              mt19937_64& prng);
void construct_simhash_sketches(const vector<shingle_vector>& shingle_vectors,
                                const vector<vector<int>>& random_vectors,
                                vector<bitset<L>>& simhash_sketches,
                                const unordered_map<uint32_t,int>& cluster_map);
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
  uint32_t chunk_length;

  // for timing
  chrono::time_point<chrono::steady_clock> start;
  chrono::time_point<chrono::steady_clock> end;
  chrono::nanoseconds diff;

  // arguments
  map<string, docopt::value> args = docopt::docopt(USAGE, { argv + 1, argv + argc });

  string edge_file(args["--edges"].asString());
  string bootstrap_file(args["--bootstrap"].asString());

  string dataset("all");

  // read bootstrap clusters and thresholds
  vector<vector<string>> clusters;
  vector<double> cluster_thresholds;
  unordered_map<string,int> cluster_map; // gid -> cluster id
  double global_threshold;

  tie(clusters, cluster_thresholds, global_threshold, chunk_length) =
    read_bootstrap_clusters(bootstrap_file);

  // FIXME: Tailored for this configuration now
  assert(K == 1 && chunk_length >= 4);

  cerr << "StreamSpot (";
  cerr << "C=" << chunk_length << ", ";
  cerr << "L=" << L << ")";

  unordered_set<string> train_gids;
  uint32_t nclusters = clusters.size();
  vector<uint32_t> cluster_sizes(nclusters);
  for (uint32_t i = 0; i < nclusters; i++) {
    cluster_sizes[i] = clusters[i].size();
    for (auto& gid : clusters[i]) {
      train_gids.insert(gid);
      cluster_map[gid] = i;
    }
  }

  //for (uint32_t i = 0; i < nclusters; i++) {
  //  cout << i << ", " << cluster_thresholds[i] << endl;
  //}

  uint32_t num_graphs; // number of training graphs
  vector<edge> train_edges;
  unordered_map<string,vector<edge>> test_edges;
  uint32_t num_test_edges;
  tie(num_graphs, train_edges, test_edges, num_test_edges) =
    read_edges(edge_file, train_gids);

#ifdef DEBUG
  for (auto& e : train_edges) {
    cout << get<0>(e) << "\t";
    cout << get<1>(e) << "\t";
    cout << get<2>(e) << "\t";
    cout << get<3>(e) << "\t";
    cout << get<4>(e) << "\t";
    cout << get<5>(e) << "" << endl;
  }
#endif

  // per-graph data structures
  unordered_map<string,graph> graphs;
  unordered_map<string,bitset<L>> streamhash_sketches;
  unordered_map<string,vector<int>> streamhash_projections;
  unordered_map<string,bitset<L>> simhash_sketches;
  unordered_map<string,shingle_vector> shingle_vectors;

  // construct bootstrap graphs offline
  cerr << "Constructing " << train_gids.size() << " training graphs:" << endl;
  for (auto& e : train_edges) {
    update_graphs(e, graphs);
  }

  // set up universal hash family for StreamHash
  allocate_random_bits(H, prng, chunk_length);

  // construct StreamHash sketches for bootstrap graphs offline
  cerr << "Constructing StreamHash sketches for training graphs:" << endl;
  for (auto& gid : train_gids) {
    unordered_map<string,uint32_t> temp_shingle_vector =
      construct_temp_shingle_vector(graphs[gid], chunk_length);
    /*cout << gid << "\t" << endl;
    for (auto& kv : temp_shingle_vector) {
      cout << kv.first << " => " << kv.second << endl;
    }*/
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
      cout << "\t" << streamhash_similarity(streamhash_sketches[gid1],
                                            streamhash_sketches[gid2]);
      cout << endl;
    }
  }
#endif

  // per-cluster data structures
  vector<vector<double>> centroid_projections;
  vector<bitset<L>> centroid_sketches;

  // construct cluster centroid sketches/projections
  cerr << "Constructing bootstrap cluster centroids:" << endl;
  tie(centroid_sketches, centroid_projections) =
    construct_centroid_sketches(streamhash_projections, clusters, nclusters);

  // compute distances of training graphs to their cluster centroids
  unordered_map<string,double> anomaly_scores;
  for (auto& gid : train_gids) {
    // streamhash sim lies between 0.0 and 1.0 so
    // angle lies between PI (least similar) and 0 (most similar)
    // anomaly score is normalised angle, lies between 1 (least similar) and 0
    double normalized_angle = 1.0 - streamhash_similarity(streamhash_sketches[gid],
                                              centroid_sketches[cluster_map[gid]]);
    anomaly_scores[gid] = normalized_angle;
  }

#ifdef DEBUG
  for (auto& kv : anomaly_scores) {
    cout << kv.first << " " << kv.second << " ";
  }
  cout << endl;
  //for (auto& s : cluster_map) {
  //  cout << s << " ";
  //}
  //cout << endl;
#endif

  // add test edges to graphs
  cerr << "Streaming in test edges:" << endl;
  uint32_t edge_num = 0;
  for (string line; getline(cin, line);) {
    string source_id;
    char source_type;
    string dest_id;
    char dest_type;
    char edge_type;
    string graph_id;

    stringstream ss(line);
    ss >> source_id >> source_type >> dest_id >> dest_type >> edge_type >> graph_id;    
    //cout << source_id << "\t";
    //cout << source_type << "\t";
    //cout << dest_id << "\t";
    //cout << dest_type << "\t";
    //cout << edge_type << "\t";
    //cout << graph_id << endl;

    edge e = make_tuple(source_id, source_type, dest_id,
                        dest_type, edge_type, graph_id);

    //
    // PROCESS EDGE
    //
    auto now = chrono::system_clock::now();
    auto epoch = now.time_since_epoch();
    auto milliseconds = chrono::duration_cast<chrono::milliseconds>(epoch);

    // update graph
    start = chrono::steady_clock::now();
    update_graphs(e, graphs);
    end = chrono::steady_clock::now();
    diff = chrono::duration_cast<chrono::nanoseconds>(end - start);
    chrono::nanoseconds graph_update_time = diff;

    // update sketches
    chrono::nanoseconds shingle_construction_time;
    chrono::nanoseconds sketch_update_time;
    vector<int> projection_delta;
    tie(projection_delta, shingle_construction_time, sketch_update_time) =
      update_streamhash_sketches(e, graphs, streamhash_sketches,
                                 streamhash_projections, chunk_length, H);

    // update centroids and centroid-graph distances
    start = chrono::steady_clock::now();
    update_distances_and_clusters(graph_id, projection_delta,
                                  streamhash_sketches,
                                  streamhash_projections,
                                  centroid_sketches, centroid_projections,
                                  cluster_sizes, cluster_map,
                                  anomaly_scores, global_threshold,
                                  cluster_thresholds);
    end = chrono::steady_clock::now();
    diff = chrono::duration_cast<chrono::nanoseconds>(end - start);
    chrono::nanoseconds cluster_update_time = diff;

    // output PAAS
    cout << "{";
    cout << "\"origin\": \"sbustreamspot\", ";
    cout << "\"uuid\": \"" << graph_id << "\", ";
    cout << "\"timestamp\": \"" << milliseconds.count() << "\", ";
    cout << "\"score\": \"" << anomaly_scores[graph_id] << "\", ";
    cout << "\"evidence\": \"cluster=" << cluster_map[graph_id] << "\"";
    cout << "}" << endl;

    edge_num++;
    cerr << "Processed edge " << edge_num << ": times ";
    cerr << "graph=" << graph_update_time.count() << "ns ";
    cerr << "sketch=" << sketch_update_time.count() << "ns ";
    cerr << "cluster=" << cluster_update_time.count() << "ns" << endl;
  }

  /*
  unordered_map<string,uint32_t> temp_shingle_vector1 =
    construct_temp_shingle_vector(graphs["cc22ab454a3511e68eb780e65016d340"],
                                  chunk_length);
  unordered_map<string,uint32_t> temp_shingle_vector2 =
    construct_temp_shingle_vector(graphs["4a0fe8be8e9e1a85fb7b44d8d0d05078"],
                                  chunk_length);
  for (auto& kv : temp_shingle_vector1)
    cout << kv.first << " => " << kv.second << endl;
  for (auto& kv : temp_shingle_vector2)
    cout << kv.first << " => " << kv.second << endl;
 
  bitset<L> streamhash_sketch1; 
  vector<int> streamhash_projection1;
  tie(streamhash_sketch1, streamhash_projection1) =
    construct_streamhash_sketch(temp_shingle_vector1, H);
  unordered_map<string,uint32_t> temp_shingle_vector2 =
    construct_temp_shingle_vector(graphs[3], chunk_length);
  for (auto& kv : temp_shingle_vector1)
    cout << kv.first << " => " << kv.second << endl;
  
    construct_shingle_vectors(shingle_vectors, shingle_id, graphs,
                            chunk_length);
  construct_random_vectors(random_vectors, 52,
                           bernoulli, prng);
  construct_simhash_sketches(shingle_vectors, random_vectors,
                             simhash_sketches, cluster_map);
  compute_similarities(shingle_vectors, simhash_sketches, streamhash_sketches,
                       cluster_map); 
  */
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
                          const vector<bitset<L>>& streamhash_sketches,
                          const unordered_map<uint32_t,int>& cluster_map) {
  for (uint32_t i = 0; i < shingle_vectors.size(); i++) {
    if (cluster_map.find(i) == cluster_map.end())
      continue;
    for (uint32_t j = 0; j < shingle_vectors.size(); j++) {
      if (cluster_map.find(j) == cluster_map.end())
        continue;
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
                                vector<bitset<L>>& simhash_sketches,
                                const unordered_map<uint32_t,int>& cluster_map) {
  // compute SimHash sketches
  for (uint32_t i = 0; i < simhash_sketches.size(); i++) {
    if (cluster_map.find(i) != cluster_map.end()) {
      construct_simhash_sketch(simhash_sketches[i], shingle_vectors[i],
                               random_vectors);
    }
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
