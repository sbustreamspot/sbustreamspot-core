/* 
 * Copyright 2016 Emaad Ahmed Manzoor
 * License: Apache License, Version 2.0
 * http://www3.cs.stonybrook.edu/~emanzoor/streamspot/
 */

#ifndef STREAMSPOT_GRAPH_H_
#define STREAMSPOT_GRAPH_H_

#include <bitset>
#include <chrono>
#include "param.h"
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace std {

// edge field indices
#define F_S               0           // source node id
#define F_STYPE           1           // source node type
#define F_D               2           // destination node id
#define F_DTYPE           3           // destination node type
#define F_ETYPE           4           // edge type
//#define F_T               5           // timestamp
#define F_GID             5           // graph id (tag)

// data structures
typedef tuple<uint32_t,char,uint32_t,char,char,uint32_t> edge;
typedef unordered_map<pair<uint32_t,char>,
                      vector<tuple<uint32_t,char,char>>> graph;
typedef vector<uint32_t> shingle_vector;

void update_graphs(edge& e, vector<graph>& graphs);
void remove_from_graph(edge& e, vector<graph>& graphs);
void print_edge(edge& e);
void print_graph(graph& g);
unordered_map<string,uint32_t>
  construct_temp_shingle_vector(const graph& g, uint32_t chunk_length);
void construct_shingle_vectors(vector<shingle_vector>& shingle_vectors,
                               unordered_map<string,uint32_t>& shingle_id,
                               vector<graph>& graphs, uint32_t chunk_length);
tuple<vector<int>, chrono::nanoseconds, chrono::nanoseconds>
update_streamhash_sketches(const edge& e, const vector<graph>& graphs,
                           vector<bitset<L>>& streamhash_sketches,
                           vector<vector<int>>& streamhash_projections,
                           uint32_t chunk_length,
                           const vector<vector<uint64_t>>& H);
double cosine_similarity(const shingle_vector& sv1, const shingle_vector& sv2);
vector<string> get_string_chunks(string s, uint32_t len);

}

#endif
