#include <cstdio>
#include <sys/mman.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define DEBUG             1
#define NODE_NAME_SIZE    20
#define TYPE_NAME_SIZE    1
#define DELIMITER         ' '

using namespace std;

void panic(string message) {
  cout << message << endl;
  exit(-1);
}

int read_edges(string filename,
               vector<pair<pair<int,int>,string>>& edges,
               unordered_map<string,int>& node_id) {
  int nverts = 0;
  int fd, i, j;
  struct stat fstatbuf;
  char *data;
  char src_name[NODE_NAME_SIZE];
  char dst_name[NODE_NAME_SIZE];
  char typ_name[NODE_NAME_SIZE];
  unordered_map<string,int>::iterator it;

  if (DEBUG)
    cout << "Reading edges from: " << filename << endl;

  fd = open(filename.c_str(), O_RDONLY);
  fstat(fd, &fstatbuf);

  // memory map the file
  data = (char*) mmap(NULL, fstatbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
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

    if (DEBUG)
      cout << "src: " << src_name
           << " dest: " << dst_name
           << " type: " << typ_name << endl;

    string u = string(src_name);
    string v = string(dst_name);
    string t = string(typ_name);

    // create a node_id for the source
    it = node_id.find(u);
    if (it == node_id.end()) { // unseen node
      node_id[u] = nverts; // allocate new id
      nverts++;
    }

    // create a node_id for the destination
    it = node_id.find(v);
    if (it == node_id.end()) { // unseen node
      node_id[v] = nverts; // allocate new id
      nverts++;
    }

    // add an edge to memory
    edges.push_back(make_pair(make_pair(node_id[u], node_id[v]), t));
  }

  close(fd);

  return nverts;
}

void print_usage() {
  cout << "USAGE: ./swoosh <GRAPH FILE>\n";
}

int main(int argc, char *argv[]) {
  unordered_map<string,int> node_id;        // map memory address to an integer
  vector<pair<pair<int,int>,string>> edges;              // edge list
  vector<vector<pair<string,string>>> G;    // adjacency list w/ edge types

  if (argc != 2) {
    print_usage();
    return -1;
  }

  read_edges(argv[1], edges, node_id);

  if (DEBUG) {
    for (unordered_map<string,int>::iterator it = node_id.begin();
         it != node_id.end();
         it++) {
      cout << "Key: " << it->first << " Value: " << it->second << endl;
    }

    for (uint32_t i = 0; i < edges.size(); i++) {
      cout << edges[i].first.first << " " << edges[i].first.second
           << " " << edges[i].second << endl;
    }
  }

  return 0;
}
