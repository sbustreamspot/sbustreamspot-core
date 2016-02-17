#ifndef STREAMSPOT_SIMHASH_H_
#define STREAMSPOT_SIMHASH_H_

#include <bitset>
#include "graph.h"
#include "param.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace std {

void construct_simhash_sketch(bitset<L>& simhash_sketch,
                              const shingle_vector& sv,
                              const vector<vector<int>>& random_vectors);
double simhash_similarity(const bitset<L>& sketch1, const bitset<L>& sketch2);

}
#endif
