#include <cmath>
#include "graph.h"
#include "hash.h"
#include <iostream>
#include "param.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace std {

void update_graphs(edge& e, vector<graph>& graphs) {
  uint32_t& src_id = get<F_S>(e);
  string& src_type = get<F_STYPE>(e);
  uint32_t& dst_id = get<F_D>(e);
  string& dst_type = get<F_DTYPE>(e);
  uint32_t& gid = get<F_GID>(e);

  if (gid + 1 > graphs.size()) { // allocate a new graph
    graphs.resize(gid + 1);
  }

  // append edge to the edge list for the source
  graphs[gid][make_pair(src_id, src_type)].push_back(e);

  // add an empty edge list for the destination
  graphs[gid].insert(make_pair(make_pair(dst_id, dst_type), vector<edge>()));
}

void print_edge(edge& e) {
  cout << "(";
  cout << get<F_S>(e) << " " << get<F_STYPE>(e) << " ";
  cout << get<F_D>(e) << " " << get<F_DTYPE>(e) << " ";
  cout << get<F_ETYPE>(e) << " " << get<F_T>(e) << " ";
  cout << get<F_GID>(e) << ")";
}

void print_graph(graph& g) {
  for (auto& kv : g) {
    cout << "(" << kv.first.first << " " << kv.first.second << ")\n";
    for (auto& e : kv.second) {
      cout << "\t";
      print_edge(e);
      cout << "\n";
    }
  }
}

void construct_shingle_vector(shingle_vector& sv,
                              unordered_set<string>& unique_shingles, graph& g) {
  for (auto& kv : g) {
    string shingle;

    // OkBFT from (src_id,type) = kv.first to construct shingle
#ifdef DEBUG
    cout << "OkBFT from " << kv.first.first << " (K = " << K << "):\n";
#endif
    queue<tuple<uint32_t,string,string>> q; // (nodeid, nodetype, edgetype)
    unordered_map<uint32_t,uint32_t> d;

    q.push(make_tuple(kv.first.first, kv.first.second, ""));
    d[kv.first.first] = 0;

    while (!q.empty()) {
      tuple<uint32_t,string,string>& node = q.front();
      uint32_t& uid = get<0>(node);
      string& utype = get<1>(node);
      string& etype = get<2>(node);
      q.pop();

#ifdef DEBUG
      cout << "\tPopped (" << uid << ", " << utype << ", " << etype << ")\n";
#endif
      // use destination and edge types to construct shingle
      shingle += etype + utype;

      if (d[uid] == K) { // node is K hops away from src_id
#ifdef DEBUG
        cout << "Hop limit reached\n";
#endif
        continue;        // don't follow its edges
      }

      // outgoing edges are already sorted by timestamp
      for (auto& e : g[make_pair(uid, utype)]) {
        uint32_t& vid = get<F_D>(e);
        string& vtype = get<F_DTYPE>(e);
        string& vetype = get<F_ETYPE>(e);

        d[vid] = d[uid] + 1;
        q.push(make_tuple(vid, vtype, vetype));
      }
    }

    sv[shingle]++; // increment shingle count
    unique_shingles.insert(shingle);
  }
}

double cosine_similarity(shingle_vector& sv1, shingle_vector& sv2) {
  double dot_product = 0.0, magnitude1 = 0.0, magnitude2 = 0.0;

  for (auto& kv : sv1) {
    magnitude1 += kv.second * kv.second;
  }
  magnitude1 = sqrt(magnitude1);
  
  for (auto& kv : sv2) {
    magnitude2 += kv.second * kv.second;
  }
  magnitude2 = sqrt(magnitude2);

  for (auto& kv : sv1) {
    uint32_t count1 = kv.second;
    uint32_t count2 = sv2.find(kv.first) == sv2.end() ? 0 : sv2[kv.first];
    dot_product += count1 * count2;
  }

  return dot_product / (magnitude1 * magnitude2);
}

}
