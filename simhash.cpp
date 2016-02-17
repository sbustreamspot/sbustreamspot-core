/* 
 * Copyright 2016 Emaad Ahmed Manzoor
 * License: Apache License, Version 2.0
 * http://www3.cs.stonybrook.edu/~emanzoor/streamspot/
 */

#include <bitset>
#include "param.h"
#include "simhash.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace std {

void construct_simhash_sketch(bitset<L>& simhash_sketch,
                              const shingle_vector& sv,
                              const vector<vector<int>>& random_vectors) {
  for (uint32_t i = 0; i < L; i++) {
    // compute i'th bit of the sketch
    int dot_product = 0;
    for (uint32_t j = 0; j < sv.size(); j++) {
      dot_product += random_vectors[i][j] * sv[j];
    }
    simhash_sketch[i] = dot_product >= 0 ? 1 : 0;
  }
}

double simhash_similarity(const bitset<L>& sketch1, const bitset<L>& sketch2) {
  // XOR = 0 if bits are the same, 1 otherwise
  return static_cast<double>((~(sketch1 ^ sketch2)).count()) / L;
}

}
