/* 
 * Copyright 2016 Emaad Ahmed Manzoor
 * License: Apache License, Version 2.0
 * http://www3.cs.stonybrook.edu/~emanzoor/streamspot/
 */

#include "hash.h"
#include <string>
#include <vector>

namespace std {

/* Universal hash from shingles (strings) to {0,1}
 *
 * Reference: Fast multilinear hashing
 *    Owen Kaser and Daniel Lemire
 *    "Strongly universal string hashing is fast."
 *    Computer Journal, 2014.
 */
int hashmulti(const string& key, const vector<uint64_t>& randbits) {
  uint64_t sum = randbits[0];
  for (uint32_t i = 0; i < key.length(); i++) {
    sum += randbits[i+1] * (static_cast<uint64_t>(key[i]) & 0xff); // sign-extension
  }
  return 2 * static_cast<int>((sum >> 63) & 1) - 1; // MSB
}

}
