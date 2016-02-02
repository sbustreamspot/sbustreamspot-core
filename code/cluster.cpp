#include <bitset>
#include "cluster.h"
#include <iostream>
#include "param.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace std {

void hash_bands(uint32_t gid, const bitset<L>& sketch,
                vector<unordered_map<bitset<R>,vector<uint32_t>>>& hash_tables) {
#ifdef DEBUG
  cout << "Hashing bands of GID: " << gid << endl;
#endif

  bitset<L> mask = bitset<L>(string(R, '1')); // R one's
  for (uint32_t i = 0; i < B; i++) {
    // get the i'th R-bit band
    string band_string = (sketch >> (R * i) & mask).to_string();
    band_string = band_string.substr(band_string.length() - R, R);
    bitset<R> band(band_string);
#ifdef DEBUG
    cout << "\tBand " << i << ": " << band.to_string() << endl;
#endif

    // hash the band to a bucket in the i'th hash table and insert the gid
    hash_tables[i][band].push_back(gid);
  }
}

bool is_isolated(const bitset<L>& sketch,
                 const vector<unordered_map<bitset<R>,vector<uint32_t>>>&
                    hash_tables) {
  bitset<L> mask = bitset<L>(string(R, '1')); // R one's
  for (uint32_t i = 0; i < B; i++) {
    string band_string = (sketch >> (R * i) & mask).to_string();
    band_string = band_string.substr(band_string.length() - R, R);
    bitset<R> band(band_string);
    if (hash_tables[i].find(band) != hash_tables[i].end()) {
      return false;
    }
  }
  return true;
}

void get_shared_bucket_graphs(const bitset<L>& sketch,
                              const vector<unordered_map<bitset<R>,
                                           vector<uint32_t>>>& hash_tables,
                              unordered_set<uint32_t>& shared_bucket_graphs) {
  bitset<L> mask = bitset<L>(string(R, '1')); // R one's
  for (uint32_t i = 0; i < B; i++) {
    // get the i'th R-bit band
    string band_string = (sketch >> (R * i) & mask).to_string();
    band_string = band_string.substr(band_string.length() - R, R);
    bitset<R> band(band_string);

    for (auto& gid : hash_tables[i].at(band)) {
      shared_bucket_graphs.insert(gid);
    }
  }
}

}
