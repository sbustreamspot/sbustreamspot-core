#include <bitset>
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

void allocate_random_bits(vector<vector<uint64_t>>&, mt19937_64);

void print_usage() {
  cout << "USAGE: ./swoosh <GRAPH FILE>\n";
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
  unordered_map<string,int> shingle_id;
  unordered_set<string> unique_shingles;

  vector<unordered_map<bitset<R>,vector<uint32_t>>> hash_tables(B);
                                                 // B hash-tables

  if (argc != 2) {
    print_usage();
    return -1;
  }

  allocate_random_bits(H, prng);
#ifdef DEBUG
    cout << "64-bit random numbers:\n";
    for (int i = 0; i < L; i++) {
      for (int j = 0; j < SMAX + 2; j++) {
        cout << H[i][j] << " ";
      }
      cout << endl;
    }
#endif

 read_edges(argv[1], edges);
#ifdef DEBUG
    for (uint32_t i = 0; i < edges.size(); i++) {
      cout << "Edge " << i << ": ";
      print_edge(edges[i]);
      cout << endl;
    }
#endif

  // main per-edge stream loop
  for (auto& e : edges) {
    update_graphs(e, graphs);
    // TODO: Update sketches/clustering too
  }

  // FIXME: Static clustering below, should be incremental

  // allocate shingle vectors
  shingle_vectors.resize(graphs.size());
  for (uint32_t i = 0; i < graphs.size(); i++) {
#ifdef DEBUG
    cout << "Constructing shingles for graph " << i << endl;
#endif
    construct_shingle_vector(shingle_vectors[i], unique_shingles, graphs[i]);
  }

#ifdef DEBUG
  for (uint32_t i = 0; i < graphs.size(); i++) {
    cout << endl;
    cout << "Graph " << i << ":\n";
    print_graph(graphs[i]);
    cout << "Shingles:\n";
    for (auto& e : shingle_vectors[i]) {
      cout << "\t" << e.first << " => " << e.second << endl;
    }
  }
#endif

  // cosine similarity between pairs of graphs
  for (uint32_t i = 0; i < graphs.size(); i++) {
    for (uint32_t j = 0; j < graphs.size(); j++) {
      double sim = cosine_similarity(shingle_vectors[i], shingle_vectors[j]);
#ifdef DEBUG
      cout << "sim(" << i << ", " << j << ") = " << sim << endl;
#endif
    }
  }
  
  // allocate shingle id's
  int current_id = 0;
  for (string shingle : unique_shingles) {
    shingle_id[shingle] = current_id;
    current_id++;
  }

#ifdef DEBUG
  cout << "Shingle ID's\n";
  for (auto kv : shingle_id) {
    cout << "\t" << kv.first << " => " << kv.second << endl;
  }
#endif

  // allocate L |S|-element {+1,-1} random vectors
  for (uint32_t i = 0; i < L; i++) {
    random_vectors[i].resize(unique_shingles.size());
    for (uint32_t j = 0; j < unique_shingles.size(); j++) {
      random_vectors[i][j] = 2 * static_cast<int>(bernoulli(prng)) - 1;
    }
  }

#ifdef DEBUG
  cout << "Random vectors:\n";
  for (uint32_t i = 0; i < L; i++) {
    cout << "\t";
    for (int rv_i : random_vectors[i]) {
      cout << rv_i << " ";
    }
    cout << endl;
  }
#endif

  // compute SimHash sketches
  simhash_sketches.resize(graphs.size());
  for (uint32_t i = 0; i < simhash_sketches.size(); i++) {
    construct_simhash_sketch(simhash_sketches[i], shingle_vectors[i],
                             shingle_id, random_vectors);
  }

#ifdef DEBUG
  cout << "SimHash sketches:\n";
  for (uint32_t i = 0; i < simhash_sketches.size(); i++) {
    cout << "\t";
    for (uint32_t j = 0; j < L; j++) {
      cout << simhash_sketches[i][j] << " ";
    }
    cout << endl;
  }
#endif

  // SimHash similarity between pairs of graphs
  for (uint32_t i = 0; i < graphs.size(); i++) {
    for (uint32_t j = 0; j < graphs.size(); j++) {
      double sim = simhash_similarity(simhash_sketches[i], simhash_sketches[j]);
#ifdef DEBUG
      cout << "simash sim(" << i << ", " << j << ") = " << sim << endl;
#endif
    }
  }

  // LSH-banding: assign graphs to hashtable buckets
  for (uint32_t i = 0; i < graphs.size(); i++) {
    hash_bands(i, simhash_sketches[i], hash_tables);
  }

#ifdef DEBUG
  cout << "Hash tables after hashing bands:\n";
  for (uint32_t i = 0; i < B; i++) {
    cout << "\tHash table " << i << ":\n";
    for (auto kv : hash_tables[i]) {
      // print graph id's in this bucket
      cout << "\t";
      for (uint32_t j = 0; j < kv.second.size(); j++) {
        cout << j << " ";
      }
      cout << endl;
    }
  }
#endif

  return 0;
}

void allocate_random_bits(vector<vector<uint64_t>>& H, mt19937_64 prng) {
  // allocate random bits for hashing
  for (uint32_t i = 0; i < L; i++) {
    // hash function h_i \in H
    H[i] = vector<uint64_t>(SMAX + 2);
    for (int j = 0; j < SMAX + 2; j++) {
      // random number m_j of h_i
      H[i][j] = prng();
    }
  }
}
