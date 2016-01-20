#ifndef SWOOSH_CLUSTER_H_
#define SWOOSH_CLUSTER_H_

#include <bitset>
#include "cluster.h"
#include "param.h"
#include <unordered_map>
#include <vector>

namespace std {

void hash_bands(uint32_t gid, bitset<L> sketch,
                vector<unordered_map<bitset<R>,vector<uint32_t>>>& hash_tables);

}

#endif
