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

#define UUID_SIZE         32

// data structures
typedef tuple<string,char,string,char,char,string> edge;
typedef unordered_map<pair<string,char>,
                      vector<tuple<string,char,char>>> graph;
typedef vector<uint32_t> shingle_vector;

void update_graphs(edge& e, unordered_map<string,graph>& graphs);
void remove_from_graph(edge& e, unordered_map<string,graph>& graphs);
void print_edge(edge& e);
void print_graph(graph& g);
unordered_map<string,uint32_t>
  construct_temp_shingle_vector(const graph& g, uint32_t chunk_length);
void construct_shingle_vectors(unordered_map<string,shingle_vector>& shingle_vectors,
                               unordered_map<string,uint32_t>& shingle_id,
                               unordered_map<string,graph>& graphs,
                               uint32_t chunk_length);
tuple<vector<int>, chrono::nanoseconds, chrono::nanoseconds>
update_streamhash_sketches(const edge& e, const unordered_map<string,graph>& graphs,
                           unordered_map<string,bitset<L>>& streamhash_sketches,
                           unordered_map<string,vector<int>>& streamhash_projections,
                           uint32_t chunk_length,
                           const vector<vector<uint64_t>>& H);
double cosine_similarity(const shingle_vector& sv1, const shingle_vector& sv2);
vector<string> get_string_chunks(string s, uint32_t len);

}

#endif
