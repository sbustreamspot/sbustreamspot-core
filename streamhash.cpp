/* 
 * Copyright 2016 Emaad Ahmed Manzoor
 * License: Apache License, Version 2.0
 * http://www3.cs.stonybrook.edu/~emanzoor/streamspot/
 */

#include <bitset>
#include "hash.h"
#include "param.h"
#include "streamhash.h"
#include <tuple>
#include <unordered_map>
#include <vector>

namespace std {

double streamhash_similarity(const bitset<L>& sketch1, const bitset<L>& sketch2) {
  // XOR = 0 if bits are the same, 1 otherwise
  return static_cast<double>((~(sketch1 ^ sketch2)).count()) / L;
}

tuple<bitset<L>,vector<int>>
construct_streamhash_sketch(const unordered_map<string,uint32_t>& shingle_vector,
                            const vector<vector<uint64_t>>& H) {
  bitset<L> sketch;
  vector<int> projection(L, 0);

  for (auto& kv : shingle_vector) {
    auto& shingle = kv.first;
    auto& count = kv.second;
    for (uint32_t i = 0; i < L; i++) {
      projection[i] += count * hashmulti(shingle, H[i]);
    }
  }

  for (uint32_t i = 0; i < L; i++) {
    sketch[i] = projection[i] >= 0 ? 1 : 0;
  }

  return make_tuple(sketch, projection);
}

}
