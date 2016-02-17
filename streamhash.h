#ifndef STREAMSPOT_STREAMHASH_H_
#define STREAMSPOT_STREAMHASH_H_

#include <bitset>
#include "param.h"
#include <tuple>
#include <unordered_map>
#include <vector>

namespace std {

double streamhash_similarity(const bitset<L>& sketch1, const bitset<L>& sketch2);
tuple<bitset<L>,vector<int>>
construct_streamhash_sketch(const unordered_map<string,uint32_t>& shingle_vector,
                            const vector<vector<uint64_t>>& H);

}
#endif
