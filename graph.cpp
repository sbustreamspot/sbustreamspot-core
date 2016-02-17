/* 
 * Copyright 2016 Emaad Ahmed Manzoor
 * License: Apache License, Version 2.0
 * http://www3.cs.stonybrook.edu/~emanzoor/streamspot/
 */

#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
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
  auto& src_id = get<F_S>(e);
  auto& src_type = get<F_STYPE>(e);
  auto& dst_id = get<F_D>(e);
  auto& dst_type = get<F_DTYPE>(e);
  auto& e_type = get<F_ETYPE>(e);
  auto& gid = get<F_GID>(e);

  // append edge to the edge list for the source
  graphs[gid][make_pair(src_id,
                        src_type)].push_back(make_tuple(dst_id,
                                                        dst_type,
                                                        e_type));
}

void remove_from_graph(edge& e, vector<graph>& graphs) {
  auto& src_id = get<F_S>(e);
  auto& src_type = get<F_STYPE>(e);
  auto& dst_id = get<F_D>(e);
  auto& dst_type = get<F_DTYPE>(e);
  auto& e_type = get<F_ETYPE>(e);
  auto& gid = get<F_GID>(e);

#ifdef DEBUG
  cout << "Removing edge: ";
  cout << src_id << " ";
  cout << src_type << " ";
  cout << dst_id << " ";
  cout << dst_type << " ";
  cout << e_type << " ";
  cout << gid << endl;
#endif

  // append edge to the edge list for the source
  auto& g = graphs[gid];
  auto node = make_pair(src_id, src_type);
  auto dest = make_tuple(dst_id, dst_type, e_type);
  auto& edge_list = g.at(node);

#ifdef DEBUG
  cout << "\tEdges from source: ";
  for (auto& x : edge_list)
    cout << get<0>(x) << " ";
  cout << endl;
#endif

  if (edge_list.size() == 1) {
    // the last edge from this node is being removed
    g.erase(node);
  } else {
    // there are other edges from this node
    auto pos = find(edge_list.begin(), edge_list.end(), dest);
    edge_list.erase(pos);
  }
}

unordered_map<string,uint32_t>
  construct_temp_shingle_vector(const graph& g, uint32_t chunk_length) {
  unordered_map<string,uint32_t> temp_shingle_vector;
  for (auto& kv : g) {
#ifdef VERBOSE
      cout << "OkBFT from " << kv.first.first << " " << kv.first.second;
      cout << " (K = " << K << ")";
      cout << " fanout = " << kv.second.size() << endl;
#endif
    string shingle; // shingle from this source node
    queue<tuple<uint32_t,char,char>> q; // (nodeid, nodetype, edgetype)
    unordered_map<uint32_t,uint32_t> d;

    q.push(make_tuple(kv.first.first, kv.first.second, ' '));
    d[kv.first.first] = 0;

    while (!q.empty()) {
      auto& node = q.front();
      auto& uid = get<0>(node);
      auto& utype = get<1>(node);
      auto& etype = get<2>(node);
      q.pop();

      // use destination and edge types to construct shingle
      shingle += etype;
      shingle += utype;

      if (d[uid] == K) { // node is K hops away from src_id
        continue;        // don't follow its edges
      }

      // outgoing edges are already sorted by timestamp
      for (auto& e : g.at(make_pair(uid, utype))) {
        auto& vid = get<0>(e);
        d[vid] = d[uid] + 1;
        q.push(e);
      }
    }

    // split shingle into chunks and increment frequency
    for (auto& chunk : get_string_chunks(shingle, chunk_length)) {
      temp_shingle_vector[chunk]++;
    }
  }

#ifdef DEBUG
  cout << "Shingles in graph:" << endl;
  for (auto& kv : temp_shingle_vector) {
    cout << kv.first << " => " << kv.second << endl;
  }
#endif

  return temp_shingle_vector;
}

void construct_shingle_vectors(vector<shingle_vector>& shingle_vectors,
                               unordered_map<string,uint32_t>& shingle_id,
                               vector<graph>& graphs, uint32_t chunk_length) {

  unordered_set<string> unique_shingles;
  vector<unordered_map<string,uint32_t>> temp_shingle_vectors(graphs.size());

  // construct a temporary shingle vector for each graph
  for (uint32_t i = 0; i < graphs.size(); i++) {
    //cout << "\tConstructing shingles for graph: " << i << endl;
    for (auto& kv : graphs[i]) {
      // OkBFT from (src_id,type) = kv.first to construct shingle
#ifdef VERBOSE
      cout << "OkBFT from " << kv.first.first << " " << kv.first.second;
      cout << " (K = " << K << ")";
      cout << " fanout = " << kv.second.size() << endl;
#endif

      string shingle; // shingle from this source node
      queue<tuple<uint32_t,char,char>> q; // (nodeid, nodetype, edgetype)
      unordered_map<uint32_t,uint32_t> d;

      q.push(make_tuple(kv.first.first, kv.first.second, ' '));
      d[kv.first.first] = 0;

      while (!q.empty()) {
        auto& node = q.front();
        auto& uid = get<0>(node);
        auto& utype = get<1>(node);
        auto& etype = get<2>(node);
        q.pop();

#ifdef VERBOSE
        cout << "\tPopped (" << uid << ", " << utype << ", " << etype << ")\n";
#endif
        // use destination and edge types to construct shingle
        shingle += etype;
        shingle += utype;

        if (d[uid] == K) { // node is K hops away from src_id
#ifdef VERBOSE
          cout << "Hop limit reached\n";
#endif
          continue;        // don't follow its edges
        }

        // outgoing edges are already sorted by timestamp
        for (auto& e : graphs[i][make_pair(uid, utype)]) {
          auto& vid = get<0>(e);
          d[vid] = d[uid] + 1;
          q.push(e);
        }
      }

      // split shingle into chunks and increment frequency
      for (auto& chunk : get_string_chunks(shingle, chunk_length)) {
        temp_shingle_vectors[i][chunk]++;
        unique_shingles.insert(chunk);
      }
    }
#ifdef DEBUG
    cout << "Shingles in graph " << i << ":\n";
    for (auto& kv : temp_shingle_vectors[i]) {
      cout << "\t" << kv.first << " => " << kv.second << endl;
    }
#endif
  }


  // use unique shingles to assign shingle id's
  uint32_t current_id = 0;
  for (auto& shingle : unique_shingles) {
    shingle_id[shingle] = current_id;
    current_id++;
  }
#ifdef DEBUG
  cout << "Shingle ID's\n";
  for (auto& kv : shingle_id) {
    cout << "\t" << kv.first << " => " << kv.second << endl;
  }
#endif

  // construct shingle vectors using shingle id's and temporary shingle vectors
  uint32_t num_unique_shingles = unique_shingles.size();
  shingle_vectors.resize(graphs.size());
  for (uint32_t i = 0; i < shingle_vectors.size(); i++) {
    shingle_vectors[i].resize(num_unique_shingles);
    for (auto& kv : temp_shingle_vectors[i]) {
      shingle_vectors[i][shingle_id[kv.first]] = kv.second;
    }
  }

#ifdef DEBUG
  cout << "Shingle vectors:\n";
  for (uint32_t i = 0; i < shingle_vectors.size(); i++) {
    cout << "\tSV for graph " << i << ": ";
    for (auto& e : shingle_vectors[i]) {
      cout << e << " ";
    }
    cout << endl;
  }
#endif
}

// FIXME: This is currently tailored for K=1
//  Adding a new edge appends 2 characters to the shingle from the source node.
//  Eg.
//    abcdabcdababab  K = 3
//    abc dab cda bab ab
//
//    New edge = pq
//    abcdabcdabababpq
//    abc dab cda bab abp q
//
//  So all the chunks require no addition/removal except the last one.
//
//  The following cases are possible (after the edge has been added):
//
//    - Last chunk length = 2
//      Hash and add chunk "et"
//    - Last chunk length = 1
//      Hash and add chunk "t"
//      Hash and add chunk (second last chunk)
//      Hash and remove chunk (second last chunk - "e")
//    - Last chunk length > 2
//      Hash and add last chunk
//      Hash and remove last chunk - "et"
tuple<vector<int>, chrono::nanoseconds, chrono::nanoseconds>
update_streamhash_sketches(const edge& e, const vector<graph>& graphs,
                           vector<bitset<L>>& streamhash_sketches,
                           vector<vector<int>>& streamhash_projections,
                           uint32_t chunk_length,
                           const vector<vector<uint64_t>>& H) {
  // source node = (src_id, src_type)
  // dst_node = (dst_id, dst_type)
  // shingle substring = (src_type, e_type, dst_type)
  //assert(K == 1 && chunk_length >= 4);

  // for timing
  chrono::time_point<chrono::steady_clock> start;
  chrono::time_point<chrono::steady_clock> end;
  chrono::microseconds shingle_construction_time;
  chrono::microseconds sketch_update_time;

  auto& src_id = get<F_S>(e);
  auto& src_type = get<F_STYPE>(e);
  auto& gid = get<F_GID>(e);

  auto& sketch = streamhash_sketches[gid];
  auto& projection = streamhash_projections[gid];
  auto& g = graphs[gid];

  start = chrono::steady_clock::now(); // start shingle construction

  // construct the last chunk
  auto& outgoing_edges = g.at(make_pair(src_id, src_type));
  uint32_t n_outgoing_edges = outgoing_edges.size();
  int shingle_length = 2 * (n_outgoing_edges + 1);
  int last_chunk_length = shingle_length - chunk_length *
                          (shingle_length/chunk_length);
  if (last_chunk_length == 0)
    last_chunk_length = chunk_length;

  string last_chunk("x", last_chunk_length);
  int len = last_chunk_length, i = n_outgoing_edges - 1;
  do {
    last_chunk[--len] = get<1>(outgoing_edges[i]); // dst_type
    if (len <= 0)
      break;
    last_chunk[--len] = get<2>(outgoing_edges[i]); // edge_type
    i--;
  } while (len > 0 && i >= 0);
  if (i < 0) {
    if (len == 2) {
      last_chunk[--len] = src_type;
    }
    if (len == 1) {
      last_chunk[--len] = ' ';
    }
  }

  // construct the second last chunk if it exists
  string sec_last_chunk("x", chunk_length);
  if (i >= 0) {
    len = chunk_length;

    if (last_chunk_length % 2 != 0) {
      sec_last_chunk[--len] = get<2>(outgoing_edges[i]); // edge_type
      i--;
    }
 
    if (i >=0 && len >= 0) {
      do {
        sec_last_chunk[--len] = get<1>(outgoing_edges[i]);
        if (len <= 0)
          break;
        sec_last_chunk[--len] = get<2>(outgoing_edges[i]);
        i--;
      } while (len > 0 && i >= 0);
    }

    if (i < 0) {
      if (len == 2) {
        sec_last_chunk[--len] = src_type;
      }
      if (len == 1) {
        sec_last_chunk[--len] = ' ';
      }
    }
  }

#ifdef DEBUG
  string shingle(" ", 1);
  shingle.reserve(2 * (n_outgoing_edges + 1));
  shingle.push_back(src_type);
  for (uint32_t i = 0; i < n_outgoing_edges; i++) {
    shingle.push_back(get<2>(outgoing_edges[i]));
    shingle.push_back(get<1>(outgoing_edges[i]));
  }

  cout << "Shingle: " << shingle << endl;
  vector<string> chunks = get_string_chunks(shingle, chunk_length);
  cout << "Last chunk: " << last_chunk << endl;
  assert(last_chunk == chunks[chunks.size() - 1]);
  if (chunks.size() > 1) {
    cout << "Second last chunk: " << sec_last_chunk << endl;
    assert(sec_last_chunk == chunks[chunks.size() - 2]);
  }
#endif

  vector<string> incoming_chunks; // to be hashed and added
  vector<string> outgoing_chunks; // to be hashed and subtracted

  incoming_chunks.push_back(last_chunk);

  if (n_outgoing_edges > 1) { // this is not the first edge
    if (last_chunk_length == 1) {
      outgoing_chunks.push_back(sec_last_chunk.substr(0,
                                                      sec_last_chunk.length() - 1));
    } else if (last_chunk_length == 2) {
      // do nothing, only incoming chunk is the last chunk
    } else { // 2 < last_chunk_length <= chunk_length, last chunk had 2 chars added
      outgoing_chunks.push_back(last_chunk.substr(0, last_chunk_length - 2));
    }
  }

  end = chrono::steady_clock::now(); // end shingle construction
  shingle_construction_time =
    chrono::duration_cast<chrono::microseconds>(end - start);

#ifdef DEBUG
  cout << "Incoming chunks: ";
  for (auto& c : incoming_chunks) {
    cout << c << ",";
  }
  cout << endl;
 
  cout << "Outgoing chunks: ";
  for (auto& c : outgoing_chunks) {
    cout << c << ",";
  }
  cout << endl;
#endif

  // record the change in the projection vector
  // this is used to update the centroid
  vector<int> projection_delta(L, 0);

  start = chrono::steady_clock::now(); // start sketch update

  // update the projection vectors
  for (auto& chunk : incoming_chunks) {
    for (uint32_t i = 0; i < L; i++) {
      int delta = hashmulti(chunk, H[i]);
      projection[i] += delta;
      projection_delta[i] += delta;
    }
  }
  for (auto& chunk : outgoing_chunks) {
    for (uint32_t i = 0; i < L; i++) {
      int delta = hashmulti(chunk, H[i]);
      projection[i] -= delta;
      projection_delta[i] -= delta;
    }
  }

  // update sketch = sign(projection)
  for (uint32_t i = 0; i < L; i++) {
    sketch[i] = projection[i] >= 0 ? 1 : 0;
  }

  end = chrono::steady_clock::now(); // end sketch update
  sketch_update_time = chrono::duration_cast<chrono::microseconds>(end - start);

  return make_tuple(projection_delta, shingle_construction_time, sketch_update_time);
}

vector<string> get_string_chunks(string s, uint32_t len) {
  vector<string> chunks;
  for (uint32_t offset = 0; offset < s.length(); offset += len) {
    chunks.push_back(s.substr(offset, len));
  }
  return chunks;
}

double cosine_similarity(const shingle_vector& sv1, const shingle_vector& sv2) {
  double dot_product = 0.0, magnitude1 = 0.0, magnitude2 = 0.0;

  uint32_t size = sv1.size();
  assert(sv1.size() == sv2.size());
  for (uint32_t i = 0; i < size; i++) {
    magnitude1 += static_cast<double>(sv1[i]) * static_cast<double>(sv1[i]);
  }

  for (uint32_t i = 0; i < size; i++) {
    magnitude2 += static_cast<double>(sv2[i]) * static_cast<double>(sv2[i]);
  }

  for (uint32_t i = 0; i < size; i++) {
    dot_product += static_cast<double>(sv1[i]) * static_cast<double>(sv2[i]);
  }

  double cosine =  dot_product / (sqrt(magnitude1) * sqrt(magnitude2));

  assert(cosine >= 0.0 && cosine <= 1.0);
  return cosine;
}

} // namespace
