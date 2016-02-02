#ifndef SWOOSH_GRAPH_H_
#define SWOOSH_GRAPH_H_

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
void print_edge(edge& e);
void print_graph(graph& g);
void construct_shingle_vectors(vector<shingle_vector>& shingle_vectors,
                               unordered_map<string,uint32_t>& shingle_id,
                               vector<graph>& graphs, uint32_t chunk_length);
double cosine_similarity(const shingle_vector& sv1, const shingle_vector& sv2);
vector<string> get_string_chunks(string s, uint32_t len);

}

#endif
