#include <bitset>
#include "param.h"
#include "simhash.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace std {

void construct_simhash_sketch(bitset<L>& simhash_sketch,
                              shingle_vector& sv,
                              unordered_map<string,uint32_t>& shingle_id,
                              vector<vector<int>>& random_vectors) {
  for (uint32_t i = 0; i < L; i++) {
    // compute i'th bit of the sketch
    int dot_product = 0;
    for (auto& kv : sv) {
      const int& id = shingle_id[kv.first];
      const int& count = static_cast<int>(kv.second);
      dot_product += random_vectors[i][id] * count;
    }

    simhash_sketch[i] = dot_product >= 0 ? 1 : 0;
  }
}

double simhash_similarity(bitset<L> sketch1, bitset<L> sketch2) {
  // XOR = 0 if bits are the same, 1 otherwise
  return static_cast<double>((~(sketch1 ^ sketch2)).count()) / L;
}

}
