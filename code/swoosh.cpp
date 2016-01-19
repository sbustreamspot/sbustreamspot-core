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
#include <unordered_set>
#include <vector>

#ifdef DEBUG
#define NDEBUG            0
#endif

#define K                 1
#define B                 10
#define R                 10          // must be a factor of L
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

void construct_simhash_sketch(bitset<L>& simhash_sketch,
                              shingle_vector& sv,
                              unordered_map<string,int>& shingle_id,
                              vector<vector<int>>& random_vectors) {
  for (uint32_t i = 0; i < L; i++) {
    // compute i'th bit of the sketch
    vector<int>& rv = random_vectors[i];
    int dot_product = 0;
    for (auto kv : sv) {
      const int& id = shingle_id[kv.first];
      const int& count = kv.second;
      dot_product += rv[id] * count;
    }
    simhash_sketch[i] = dot_product >= 0 ? 1 : 0;
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

double simhash_similarity(bitset<L> sketch1, bitset<L> sketch2) {
  // XOR = 0 if bits are the same, 1 otherwise
  return static_cast<double>((~(sketch1 ^ sketch2)).count()) / L;
}

void hash_bands(uint32_t gid, bitset<L> sketch,
                vector<unordered_map<bitset<R>,vector<uint32_t>>>& hash_tables) {
  // divide L-bit sketch into B R-bit chunks
  assert(L == B * R);

#ifdef DEBUG
  cout << "Hashing bands of GID: " << gid << endl;
  cout << sketch.to_string() << endl;
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
  bernoulli_distribution bernoulli(0.5);         // to generate random vectors
  vector<vector<int>> random_vectors(L);         // |S|-element random vectors
  vector<bitset<L>> simhash_sketches;

  vector<shingle_vector> shingle_vectors;        // |S|-element shingle vectors
  unordered_map<string,int> shingle_id;
  unordered_set<string> unique_shingles;

  vector<unordered_map<bitset<R>,vector<uint32_t>>> hash_tables(B);
                                                 // B hash-tables

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

  // allocate shingle vectors
  shingle_vectors.resize(graphs.size());
  for (uint32_t i = 0; i < graphs.size(); i++) {
#ifdef DEBUG
    cout << "Constructing shingles for graph " << i << endl;
#endif
    construct_shingle_vector(shingle_vectors[i], unique_shingles, graphs[i]);
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
  
  // allocate shingle id's
  int current_id = 0;
  for (string shingle : unique_shingles) {
    shingle_id[shingle] = current_id;
    current_id++;
  }

#ifdef DEBUG
  cout << "Single ID's\n";
  for (auto kv : shingle_id) {
    cout << "\t" << kv.first << " => " << kv.second << endl;
  }
#endif

  // allocate L |S|-element {+1,-1} random vectors
  for (uint32_t i = 0; i < L; i++) {
    random_vectors[i].resize(unique_shingles.size());
    for (uint32_t j = 0; j < unique_shingles.size(); j++) {
      random_vectors[i][j] = 2 * static_cast<int>(bernoulli(prng)) - 1;
    }
  }

#ifdef DEBUG
  cout << "Random vectors:\n";
  for (uint32_t i = 0; i < L; i++) {
    cout << "\t";
    for (int rv_i : random_vectors[i]) {
      cout << rv_i << " ";
    }
    cout << endl;
  }
#endif

  // compute SimHash sketches
  simhash_sketches.resize(graphs.size());
  for (uint32_t i = 0; i < simhash_sketches.size(); i++) {
    construct_simhash_sketch(simhash_sketches[i], shingle_vectors[i],
                             shingle_id, random_vectors);
  }

#ifdef DEBUG
  cout << "SimHash sketches:\n";
  for (uint32_t i = 0; i < simhash_sketches.size(); i++) {
    cout << "\t";
    for (uint32_t j = 0; j < L; j++) {
      cout << simhash_sketches[i][j] << " ";
    }
    cout << endl;
  }
#endif

  // SimHash similarity between pairs of graphs
  for (uint32_t i = 0; i < graphs.size(); i++) {
    for (uint32_t j = 0; j < graphs.size(); j++) {
      double sim = simhash_similarity(simhash_sketches[i], simhash_sketches[j]);
#ifdef DEBUG
      cout << "simash sim(" << i << ", " << j << ") = " << sim << endl;
#endif
    }
  }

  // LSH-banding: assign graphs to hashtable buckets
  for (uint32_t i = 0; i < graphs.size(); i++) {
    hash_bands(i, simhash_sketches[i], hash_tables);
  }

#ifdef DEBUG
  cout << "Hash tables after hashing bands:\n";
  for (uint32_t i = 0; i < B; i++) {
    cout << "\tHash table " << i << ":\n";
    for (auto kv : hash_tables[i]) {
      // print graph id's in this bucket
      cout << "\t";
      for (uint32_t j = 0; j < kv.second.size(); j++) {
        cout << j << " ";
      }
      cout << endl;
    }
  }
#endif

  return 0;
}
