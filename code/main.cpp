#include <bitset>
#include <cassert>
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

using namespace std;

void allocate_random_bits(vector<vector<uint64_t>>&, mt19937_64&);
void compute_similarities(vector<shingle_vector>& shingle_vectors,
                          vector<bitset<L>>& simhash_sketches);
void construct_random_vectors(vector<vector<int>>& random_vectors,
                              uint32_t rvsize,
                              bernoulli_distribution& bernoulli,
                              mt19937_64& prng);
void construct_simhash_sketches(vector<shingle_vector>& shingle_vectors,
                                vector<vector<int>>& random_vectors,
                                vector<bitset<L>>& simhash_sketches);
void perform_lsh_banding(vector<bitset<L>>& simhash_sketches,
                         vector<unordered_map<bitset<R>,vector<uint32_t>>>&
                            hash_tables);
void print_lsh_clusters(vector<bitset<L>>& simhash_sketches,
                         vector<unordered_map<bitset<R>,vector<uint32_t>>>&
                            hash_tables);

void print_usage() {
  cout << "USAGE: ./swoosh <edge file> <chunk length>\n";
}

int main(int argc, char *argv[]) {
  vector<edge> edges;                            // edge list
  vector<graph> graphs;                          // graph list

  bitset<L> sketch;                              // L-bit sketch (initially 0)
  vector<int> projection(L);                     // projection vector
  vector<vector<uint64_t>> H(L);                 // Universal family H, contains
                                                 // L hash functions, each
                                                 // represented by SMAX+2 64-bit
                                                 // random integers

  mt19937_64 prng(SEED);                         // Mersenne Twister 64-bit PRNG
  bernoulli_distribution bernoulli(0.5);         // to generate random vectors
  vector<vector<int>> random_vectors(L);         // |S|-element random vectors
  vector<bitset<L>> simhash_sketches;

  vector<shingle_vector> shingle_vectors;        // |S|-element shingle vectors
  unordered_map<string,uint32_t> shingle_id;
  unordered_set<string> unique_shingles;

  vector<unordered_map<bitset<R>,vector<uint32_t>>> hash_tables(B);
                                                 // B hash-tables

  if (argc != 3) {
    print_usage();
    return -1;
  }

  // arguments
  string edge_file(argv[1]);
  uint32_t chunk_length = atoi(argv[2]);

  //allocate_random_bits(H, prng);
  uint32_t num_graphs = read_edges(edge_file, edges);

  cout << "Constructing " << num_graphs << " graphs:" << endl;
  graphs.resize(num_graphs);
  for (auto& e : edges) {
    update_graphs(e, graphs); // TODO: Update sketches/clustering too
  }

  cout << "Constructing shingle vectors:" << endl;
  construct_shingle_vectors(shingle_vectors, shingle_id, graphs,
                            chunk_length);

  cout << "Constructing Simhash sketches:" << endl;
  construct_random_vectors(random_vectors, shingle_vectors[0].size(),
                           bernoulli, prng);
  construct_simhash_sketches(shingle_vectors, random_vectors,
                             simhash_sketches);

  cout << "Computing pairwise similarities:" << endl;
  compute_similarities(shingle_vectors, simhash_sketches);

  cout << "LSH banding:" << endl;
  perform_lsh_banding(simhash_sketches, hash_tables);

  cout << "Print LSH clusters:" << endl;
  print_lsh_clusters(simhash_sketches, hash_tables);

  return 0;
}

void allocate_random_bits(vector<vector<uint64_t>>& H, mt19937_64& prng) {
  // allocate random bits for hashing
  for (uint32_t i = 0; i < L; i++) {
    // hash function h_i \in H
    H[i] = vector<uint64_t>(SMAX + 2);
    for (int j = 0; j < SMAX + 2; j++) {
      // random number m_j of h_i
      H[i][j] = prng();
    }
  }
#ifdef DEBUG
    cout << "64-bit random numbers:\n";
    for (int i = 0; i < L; i++) {
      for (int j = 0; j < SMAX + 2; j++) {
        cout << H[i][j] << " ";
      }
      cout << endl;
    }
#endif
}

void compute_similarities(vector<shingle_vector>& shingle_vectors,
                          vector<bitset<L>>& simhash_sketches) {
  for (uint32_t i = 0; i < shingle_vectors.size(); i++) {
    for (uint32_t j = 0; j < shingle_vectors.size(); j++) {
      double cosine = cosine_similarity(shingle_vectors[i],
                                        shingle_vectors[j]);
      double angsim = 1 - acos(cosine)/PI;
      double hashsim = simhash_similarity(simhash_sketches[i], simhash_sketches[j]);
      double diff = abs(angsim - hashsim)/angsim;
      cout << i << "\t" << j << "\t";
      cout << cosine;
#ifdef DEBUG
      cout << "\t" << angsim << "\t" << hashsim;
      cout << "\t" << diff;
#endif
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

void construct_simhash_sketches(vector<shingle_vector>& shingle_vectors,
                                vector<vector<int>>& random_vectors,
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

void perform_lsh_banding(vector<bitset<L>>& simhash_sketches,
                         vector<unordered_map<bitset<R>,vector<uint32_t>>>&
                            hash_tables) {
  // LSH-banding: assign graphs to hashtable buckets
  for (uint32_t i = 0; i < simhash_sketches.size(); i++) {
    hash_bands(i, simhash_sketches[i], hash_tables);
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

void print_lsh_clusters(vector<bitset<L>>& simhash_sketches,
                         vector<unordered_map<bitset<R>,vector<uint32_t>>>&
                            hash_tables) {
#ifdef DEBUG
  cout << "Printing LSH clusters:" << endl;
#endif

  unordered_set<uint32_t> graphs(simhash_sketches.size());
  for (uint32_t i = 0; i < simhash_sketches.size(); i++) {
    graphs.insert(i);
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

    cout << "\tCluster: {";
    for (auto& e : cluster) {
      cout << " " << e;
    }
    cout << " } Size: " << cluster.size() << endl;

    for (auto& e : cluster) {
      graphs.erase(e);
    }
  }
}
