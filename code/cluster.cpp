#include <bitset>
#include <cassert>
#include "cluster.h"
#include <iostream>
#include "param.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace std {

void hash_bands(uint32_t gid, bitset<L>& sketch,
                vector<unordered_map<bitset<R>,vector<uint32_t>>>& hash_tables) {
  // divide L-bit sketch into B R-bit chunks
  assert(L == B * R);

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

}
