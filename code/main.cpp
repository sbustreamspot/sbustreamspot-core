#include <bitset>
#include <cassert>
#include <chrono>
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
  cout << "USAGE: ./swoosh <edge file> <chunk length>\n";
}

int main(int argc, char *argv[]) {
  vector<edge> edges;                            // edge list
  vector<graph> graphs;                          // graph list

  vector<vector<uint64_t>> H(L);                 // Universal family H, contains
                                                 // L hash functions, each
                                                 // represented by chunk_length+2
                                                 // 64-bit random integers

  mt19937_64 prng(SEED);                         // Mersenne Twister 64-bit PRNG
  bernoulli_distribution bernoulli(0.5);         // to generate random vectors
  vector<vector<int>> random_vectors(L);         // |S|-element random vectors
  vector<bitset<L>> simhash_sketches;
  vector<bitset<L>> streamhash_sketches;
  vector<vector<int>> streamhash_projections;

  vector<shingle_vector> shingle_vectors;        // |S|-element shingle vectors
  unordered_map<string,uint32_t> shingle_id;
  unordered_set<string> unique_shingles;

  //vector<unordered_map<bitset<R>,vector<uint32_t>>> hash_tables(B);

  // for timing
  chrono::time_point<chrono::steady_clock> start;
  chrono::time_point<chrono::steady_clock> end;
  chrono::microseconds diff;

  if (argc != 3) {
    print_usage();
    return -1;
  }

  // arguments
  string edge_file(argv[1]);
  uint32_t chunk_length = atoi(argv[2]);
  cout << "Executing with chunk length: " << chunk_length << endl;
  
  // FIXME: Tailored for this configuration now
  assert(K == 1 && chunk_length >= 4);

  allocate_random_bits(H, prng, chunk_length);
  uint32_t num_graphs = read_edges(edge_file, edges);

  // add edges to graphs
  cout << "Constructing " << num_graphs << " graphs ";
  cout << "and StreamHash sketches: " << endl;

  graphs.resize(num_graphs);
  streamhash_sketches.resize(num_graphs);
  streamhash_projections = vector<vector<int>>(num_graphs,
                                               vector<int>(L,0));

  start = chrono::steady_clock::now();
  for (uint32_t i = 0; i < edges.size(); i++) {
    update_graphs(edges[i], graphs);
    update_streamhash_sketches(edges[i], graphs, streamhash_sketches,
                               streamhash_projections, chunk_length, H);
  }
  end = chrono::steady_clock::now();
  diff = chrono::duration_cast<chrono::microseconds>(end - start);
  cout << "\tGraph + StreamHash sketch update (per-edge): ";
  cout << static_cast<double>(diff.count())/edges.size() << "us" << endl;

  cout << "Constructing shingle vectors:" << endl;
  start = chrono::steady_clock::now();
  construct_shingle_vectors(shingle_vectors, shingle_id, graphs,
                            chunk_length);
  end = chrono::steady_clock::now();
  diff = chrono::duration_cast<chrono::microseconds>(end - start);
  cout << "\tShingle vector construction (per-graph): ";
  cout << static_cast<double>(diff.count())/num_graphs << "us" << endl;

  cout << "Constructing Simhash sketches:" << endl;
  construct_random_vectors(random_vectors, shingle_vectors[0].size(),
                           bernoulli, prng);
  construct_simhash_sketches(shingle_vectors, random_vectors,
                             simhash_sketches);

  cout << "Computing pairwise similarities:" << endl;
  compute_similarities(shingle_vectors, simhash_sketches, streamhash_sketches);

  /*// label attack/normal graph id's
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
      cout << "\t" << simhash_sim;
      cout << "\t" << streamhash_sim;
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
  simhash_sketches.resize(shingle_vectors.size());
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
