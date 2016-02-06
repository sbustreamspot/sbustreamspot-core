#ifndef SWOOSH_IO_H_
#define SWOOSH_IO_H_

#include "graph.h"
#include <string>
#include <tuple>
#include <vector>

namespace std {

tuple<uint32_t,vector<edge>,vector<edge>>
  read_edges(string filename, const unordered_set<uint32_t>& train_gids);
tuple<vector<vector<uint32_t>>, vector<double>, double>
  read_bootstrap_clusters(string bootstrap_file);

}

#endif
