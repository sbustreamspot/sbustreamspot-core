#include <bitset>
#include <cassert>
#include <cstdio>
#include <sys/mman.h>
#include <fcntl.h>
#include <iostream>
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

#define BUF_SIZE          50
#define DELIMITER         '\t'
#define L                 100
#define SMAX              32
#define SEED              23

using namespace std;

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

void read_edges(string filename,
                vector<tuple<uint32_t,string,
                             uint32_t,string,
                             string,uint64_t,uint32_t>>& edges) {
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

void print_usage() {
  cout << "USAGE: ./swoosh <GRAPH FILE>\n";
}

int main(int argc, char *argv[]) {
  vector<tuple<uint32_t,string,uint32_t,string,
               string,uint64_t,uint32_t>> edges; // edge list
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
      cout << "Edge " << i << ": (";
      cout << get<0>(edges[i]) << " " << get<1>(edges[i]) << " ";
      cout << get<2>(edges[i]) << " " << get<3>(edges[i]) << " ";
      cout << get<4>(edges[i]) << " " << get<5>(edges[i]) << " ";
      cout << get<6>(edges[i]) << ")" << endl;
    }
#endif

  return 0;
}
