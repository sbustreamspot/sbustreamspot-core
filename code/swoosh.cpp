#include <bitset>
#include <cassert>
#include <cstdio>
#include <sys/mman.h>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#ifdef DEBUG
#define NDEBUG            0
#endif

#define NODE_NAME_SIZE    20
#define TYPE_NAME_SIZE    1
#define DELIMITER         ' '
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
                vector<pair<pair<string,string>,string>>& edges) {
  int fd;
  uint32_t i, j;
  struct stat fstatbuf;
  char *data;
  char src_name[NODE_NAME_SIZE];
  char dst_name[NODE_NAME_SIZE];
  char typ_name[NODE_NAME_SIZE];
  unordered_map<string,int>::iterator it;

  fd = open(filename.c_str(), O_RDONLY);
  fstat(fd, &fstatbuf);

  // memory map the file
  data = (char*) mmap(NULL, fstatbuf.st_size, PROT_READ, MAP_PRIVATE|MAP_POPULATE,
                      fd, 0);
  madvise(data, fstatbuf.st_size, MADV_SEQUENTIAL);

  if (data < 0) { // mmap failed
    panic("mmap'ing graph file failed");
    close(fd);
  }

  // read edges from the file
  i = 0;
  while (i < fstatbuf.st_size) {
    // field 1: source name
    j = 0;
    while (data[i] != DELIMITER) {
      src_name[j] = data[i];
      i++;
      j++;
    }
    src_name[j] = '\0';

    i++; // skip delimiter

    // field 2: destination name
    j = 0;
    while (data[i] != DELIMITER) {
      dst_name[j] = data[i];
      i++;
      j++;
    }
    dst_name[j] = '\0';

    i++; // skip delimiter

    // field 3: type name
    j = 0;
    while (data[i] != '\n') {
      typ_name[j] = data[i];
      i++;
      j++;
    }
    typ_name[j] = '\0';

    i++; // skip newline

    // convert source, destination and type names to 32-bit strings
    /*assert(strlen(src_name) > 0);
    uint32_t k;
    u32string u32;
    for (j = 0; j < (strlen(src_name) - 1)/4 + 1; j++) {
      uint32_t byte = 0;
      for (k = 0; k < 4 && j * 4 + k < strlen(src_name); k++) {
        byte |= (src_name[j * 4 + k] << 8 * k);
      }
      u32 += byte;
    }

    assert(strlen(dst_name) > 0);
    u32string v32;
    for (j = 0; j < (strlen(dst_name) - 1)/4 + 1; j++) {
      uint32_t byte = 0;
      for (k = 0; k < 4 && j * 4 + k < strlen(dst_name); k++) {
        byte |= (dst_name[j * 4 + k] << 8 * k);
      }
      v32 += byte;
    }

    assert(strlen(typ_name) > 0);
    u32string t32;
    for (j = 0; j < (strlen(typ_name) - 1)/4 + 1; j++) {
      uint32_t byte = 0;
      for (k = 0; k < 4 && j * 4 + k < strlen(typ_name); k++) {
        byte |= (typ_name[j * 4 + k] << 8 * k);
      }
      t32 += byte;
    }*/

    // add an edge to memory
    edges.push_back(make_pair(make_pair(string(src_name), string(dst_name)),
                              string(typ_name)));
  }

  close(fd);
}

void print_usage() {
  cout << "USAGE: ./swoosh <GRAPH FILE>\n";
}

int main(int argc, char *argv[]) {
  vector<pair<pair<string,string>,string>> edges;
                                            // edge list
  bitset<L> sketch;                         // L-bit sketch (initially 0)
  vector<int> projection(L);                // projection vector
  vector<vector<uint64_t>> H(L);            // Universal family H, contains
                                            // L hash functions, each represented by
                                            // SMAX+2 64-bit random integers
  mt19937_64 prng(SEED);                    // Mersenne Twister 64-bit PRNG

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
      cout << edges[i].first.first
           << " hash: " << static_cast<int>(hashmulti(edges[i].first.first, H[0]))
           << " " << edges[i].first.second
           << " hash: " << static_cast<int>(hashmulti(edges[i].first.second, H[0]))
           << " " << edges[i].second
           << " hash: " << static_cast<int>(hashmulti(edges[i].second, H[0]))
           << endl;
    }
#endif

  return 0;
}
