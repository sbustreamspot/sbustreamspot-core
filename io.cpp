/* 
 * Copyright 2016 Emaad Ahmed Manzoor
 * License: Apache License, Version 2.0
 * http://www3.cs.stonybrook.edu/~emanzoor/streamspot/
 */

#include <cassert>
#include <fcntl.h>
#include <fstream>
#include "graph.h"
#include "io.h"
#include <iostream>
#include "param.h"
#include <string>
#include <sstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <tuple>
#include <unistd.h>
#include "util.h"
#include <vector>

namespace std {

tuple<uint32_t,vector<edge>,unordered_map<string,vector<edge>>, uint32_t>
  read_edges(string filename, const unordered_set<string>& train_gids) {
  // read edges into memory
  cerr << "Reading training edges from: " << filename << endl;

  vector<edge> train_edges;
  unordered_map<string,vector<edge>> test_edges;
  uint32_t num_test_edges = 0;
  uint32_t num_train_edges = 0;

  // get file size
  struct stat fstatbuf;
  int fd = open(filename.c_str(), O_RDONLY);
  fstat(fd, &fstatbuf);

  // memory map the file
#ifdef __linux__
  static const int mmap_flags = MAP_PRIVATE|MAP_POPULATE;
#else
  static const int mmap_flags = MAP_PRIVATE;
#endif
  char *data = (char*) mmap(NULL, fstatbuf.st_size, PROT_READ,
                            mmap_flags, fd, 0);
  madvise(data, fstatbuf.st_size, MADV_SEQUENTIAL);

  if (data < 0) { // mmap failed
    panic("mmap'ing graph file failed");
    close(fd);
    exit(-1);
  }

  // read edges from the file
  uint32_t i = 0;
  uint32_t line = 0;
  unordered_set<string> graph_ids; 
  char src_type, dst_type, e_type;
  while (i < fstatbuf.st_size) {
    // field 1: source id
    string src_id(' ', UUID_SIZE);
    for (uint32_t j = 0; j < UUID_SIZE; j++, i++) {
      src_id[j] = data[i];
    }

    i++; // skip delimiter

    // field 2: source type
    src_type = data[i];
    i += 2; // skip delimiter

    // field 3: dest id
    string dst_id(' ', UUID_SIZE);
    for (uint32_t j = 0; j < UUID_SIZE; j++, i++) {
      dst_id[j] = data[i];
    }

    i++; // skip delimiter

    // field 4: dest type
    dst_type = data[i];
    i += 2; // skip delimiter

    // field 5: edge type
    e_type = data[i];
    i += 2; // skip delimiter

    // field 7: graph id
    string graph_id(' ', UUID_SIZE);
    for (uint32_t j = 0; j < UUID_SIZE; j++, i++) {
      graph_id[j] = data[i];
    }

    graph_ids.insert(graph_id);

    i++; // skip newline

    // add an edge to memory
    assert (train_gids.find(graph_id) != train_gids.end());
    if (train_gids.find(graph_id) != train_gids.end()) {
      train_edges.push_back(make_tuple(src_id, src_type,
                                       dst_id, dst_type,
                                       e_type, graph_id));
      num_train_edges++;
    } else {
      test_edges[graph_id].push_back(make_tuple(src_id, src_type,
                                                dst_id, dst_type,
                                                e_type, graph_id));
      num_test_edges++;
    }

    line++;
  }

  close(fd);

#ifdef VERBOSE
  for (uint32_t i = 0; i < edges.size(); i++) {
    cout << "Edge " << i << ": ";
    print_edge(edges[i]);
    cout << endl;
  }
  cout << "Dropped edges: " << num_dropped_edges << endl;
  cout << "Train edges: " << num_train_edges << endl;
  cout << "Test edges: " << num_test_edges << endl;
#endif

  return make_tuple(graph_ids.size(), train_edges, test_edges, num_test_edges);
}

tuple<vector<vector<string>>, vector<double>, double, uint32_t>
  read_bootstrap_clusters(string bootstrap_file) {
  int nclusters;
  double global_threshold;
  uint32_t chunk_length;
  ifstream f(bootstrap_file);
  string line;
  stringstream ss;

  getline(f, line);
  ss.str(line);
  ss >> nclusters >> global_threshold >> chunk_length;
  vector<double> cluster_thresholds(nclusters);
  vector<vector<string>> clusters(nclusters);

  for (int i = 0; i < nclusters; i++) {
    getline(f, line);
    ss.clear();
    ss.str(line);

    double cluster_threshold;
    ss >> cluster_threshold;
    cluster_thresholds[i] = cluster_threshold;

    string gid;
    while (ss >> gid) {
      clusters[i].push_back(gid);
    }
  }

  return make_tuple(clusters, cluster_thresholds, global_threshold, chunk_length);
}

}
