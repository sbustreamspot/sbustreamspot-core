#ifndef SWOOSH_SIMHASH_H_
#define SWOOSH_SIMHASH_H_

#include <bitset>
#include "graph.h"
#include "param.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace std {

void construct_simhash_sketch(bitset<L>& simhash_sketch,
                              shingle_vector& sv,
                              unordered_map<string,uint32_t>& shingle_id,
                              vector<vector<int>>& random_vectors);
double simhash_similarity(bitset<L> sketch1, bitset<L> sketch2);

}
#endif
