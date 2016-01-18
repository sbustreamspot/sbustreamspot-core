#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <sys/mman.h>
#include <functional>
#include <fcntl.h>
#include <iostream>
#include <queue>
#include <random>
#include <string>
#include <sys/stat.h>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#ifdef DEBUG
#define NDEBUG            0
#endif

#define K                 1
#define BUF_SIZE          50
#define DELIMITER         '\t'
#define L                 100
#define SMAX              32
#define SEED              23

// edge field indices
#define F_S               0           // source node id
#define F_STYPE           1           // source node type
#define F_D               2           // destination node id
#define F_DTYPE           3           // destination node type
#define F_ETYPE           4           // edge type
#define F_T               5           // timestamp
#define F_GID             6           // graph id (tag)

using namespace std;

typedef tuple<uint32_t,string,uint32_t,string,string,uint64_t,uint32_t> edge;
typedef unordered_map<pair<uint32_t,string>,vector<edge>> graph;
typedef unordered_map<string,uint32_t> shingle_vector;

/* Combination hash from Boost */
template <class T>
inline void hash_combine(size_t& seed, const T& v)
{
    hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std
{
  template<typename S, typename T> struct hash<pair<S, T>>
  {
    inline size_t operator()(const pair<S, T>& v) const
    {
      size_t seed = 0;
      ::hash_combine(seed, v.first);
      ::hash_combine(seed, v.second);
      return seed;
    }
  };
}
/* End combination hash from Boost */

void panic(string message) {
  cout << message << endl;
  exit(-1);
}

/* Universal hash from shingle (strings) to {0,1}
 *
 * Reference: Fast multilinear hashing
 *    Owen Kaser and Daniel Lemire
 *    "Strongly universal string hashing is fast."
 *    Computer Journal, 2014.
 */
uint8_t hashmulti(const string& key, vector<uint64_t>& randbits) {
  uint64_t sum = randbits[0];
  for (uint32_t i = 0; i < key.length(); i++) {
    sum += randbits[i+1] * (static_cast<uint64_t>(key[i]) & 0xff); // sign-extension
  }
#ifdef DEBUG
  cout << "key = " << key << ", hash = " << hex << sum << endl;
#endif
  return static_cast<uint8_t>((sum >> 63) & 1); // MSB
}

void read_edges(string filename, vector<edge>& edges) {
  // get file size
  struct stat fstatbuf;
  int fd = open(filename.c_str(), O_RDONLY);
  fstat(fd, &fstatbuf);

  // memory map the file
  char *data = (char*) mmap(NULL, fstatbuf.st_size, PROT_READ,
                            MAP_PRIVATE|MAP_POPULATE, fd, 0);
  madvise(data, fstatbuf.st_size, MADV_SEQUENTIAL);

  if (data < 0) { // mmap failed
    panic("mmap'ing graph file failed");
    close(fd);
  }

  // read edges from the file
  uint32_t i = 0, j = 0;
  while (i < fstatbuf.st_size) {
    // field 1: source id
    uint32_t src_id = data[i] - '0';
    while (data[++i] != DELIMITER) {
      src_id = src_id * 10 + (data[i] - '0');
    }

    i++; // skip delimiter

    // field 2: source type
    char src_type[BUF_SIZE] = {0};
    j = 0;
    while (data[i] != DELIMITER) {
      src_type[j++] = data[i++];
    }
    src_type[j] = '\0';

    i++; // skip delimiter

    // field 3: dest id
    uint32_t dst_id = data[i] - '0';
    while (data[++i] != DELIMITER) {
      dst_id = dst_id * 10 + (data[i] - '0');
    }

    i++; // skip delimiter

    // field 4: dest type
    char dst_type[BUF_SIZE] = {0};
    j = 0;
    while (data[i] != DELIMITER) {
      dst_type[j++] = data[i++];
    }
    dst_type[j] = '\0';

    i++; // skip delimiter

    // field 5: edge type
    char e_type[BUF_SIZE] = {0};
    j = 0;
    while (data[i] != DELIMITER) {
      e_type[j++] = data[i++];
    }
    e_type[j] = '\0';

    i++; // skip delimiter

    // field 6: timestamp
    uint64_t ts = data[i] - '0';
    while (data[++i] != DELIMITER) {
      ts = ts * 10 + (data[i] - '0');
    }

    i++; // skip delimiter

    // field 7: graph id
    uint32_t graph_id = data[i] - '0';
    while (data[++i] != '\n') {
      graph_id = graph_id * 10 + (data[i] - '0');
    }

    i++; // skip newline

    // add an edge to memory
    edges.push_back(make_tuple(src_id, string(src_type),
                               dst_id, string(dst_type),
                               string(e_type), ts, graph_id));
  }

  close(fd);
}

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

void print_usage() {
  cout << "USAGE: ./swoosh <GRAPH FILE>\n";
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

void construct_shingle_vector(shingle_vector& sv, graph& g) {
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

int main(int argc, char *argv[]) {
  vector<edge> edges;                            // edge list
  vector<graph> graphs;                          // graph list
  bitset<L> sketch;                              // L-bit sketch (initially 0)
  vector<int> projection(L);                     // projection vector
  vector<vector<uint64_t>> H(L);                 // Universal family H, contains
                                                 // L hash functions, each
                                                 // represented by SMAX+2 64-bit
                                                 // random integers
  mt19937_64 prng(SEED);                         // Mersenne Twister 64-bit PRNG

  if (argc != 2) {
    print_usage();
    return -1;
  }

  // allocate random bits for hashing
  for (int i = 0; i < L; i++) {
    // hash function h_i \in H
    H[i] = vector<uint64_t>(SMAX + 2);
    for (int j = 0; j < SMAX + 2; j++) {
      // random number m_j of h_i
      H[i][j] = prng();
    }
  }

#ifdef DEBUG
    cout << "64-bit random numbers:\n";
    for (int i = 0; i < L; i++) {
      for (int j = 0; j < SMAX + 2; j++) {
        cout << H[i][j] << " ";
      }
      cout << endl;
    }
#endif

  // read edges into memory
#ifdef DEBUG
    cout << "Reading edges from: " << argv[1] << endl;
#endif
  read_edges(argv[1], edges);

#ifdef DEBUG
    for (uint32_t i = 0; i < edges.size(); i++) {
      cout << "Edge " << i << ": ";
      print_edge(edges[i]);
      cout << endl;
    }
#endif

  // main per-edge stream loop
  for (auto& e : edges) {
    update_graphs(e, graphs);
    // TODO: Update sketches/clustering too
  }

  // FIXME: Static clustering below, should be incremental
  vector<shingle_vector> shingle_vectors(graphs.size());
  for (uint32_t i = 0; i < graphs.size(); i++) {
#ifdef DEBUG
    cout << "Constructing shingles for graph " << i << endl;
#endif
    construct_shingle_vector(shingle_vectors[i], graphs[i]);
  }

#ifdef DEBUG
  for (uint32_t i = 0; i < graphs.size(); i++) {
    cout << endl;
    cout << "Graph " << i << ":\n";
    print_graph(graphs[i]);
    cout << "Shingles:\n";
    for (auto& e : shingle_vectors[i]) {
      cout << "\t" << e.first << " => " << e.second << endl;
    }
  }
#endif

  // cosine similarity between pairs of graphs
  for (uint32_t i = 0; i < graphs.size(); i++) {
    for (uint32_t j = 0; j < graphs.size(); j++) {
      double sim = cosine_similarity(shingle_vectors[i], shingle_vectors[j]);
#ifdef DEBUG
      cout << "sim(" << i << ", " << j << ") = " << sim << endl;
#endif
    }
  }

  return 0;
}
