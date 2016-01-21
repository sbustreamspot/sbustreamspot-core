#include <fcntl.h>
#include "graph.h"
#include "io.h"
#include <iostream>
#include "param.h"
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "util.h"
#include <vector>

namespace std {

void read_edges(string filename, vector<edge>& edges) {
  // read edges into memory
  cout << "Reading edges from: " << filename << endl;

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
  uint32_t i = 0;
  uint32_t line = 0;
  char src_type, dst_type, e_type;
  while (i < fstatbuf.st_size) {
    // field 1: source id
    uint32_t src_id = data[i] - '0';
    while (data[++i] != DELIMITER) {
      src_id = src_id * 10 + (data[i] - '0');
    }

    i++; // skip delimiter

    // field 2: source type
    src_type = data[i];
    i += 2; // skip delimiter

    // field 3: dest id
    uint32_t dst_id = data[i] - '0';
    while (data[++i] != DELIMITER) {
      dst_id = dst_id * 10 + (data[i] - '0');
    }
    i++; // skip delimiter

    // field 4: dest type
    dst_type = data[i];
    i += 2; // skip delimiter

    // field 5: edge type
    e_type = data[i];
    i += 2; // skip delimiter

    // field 7: graph id
    uint32_t graph_id = data[i] - '0';
    while (data[++i] != '\n') {
      graph_id = graph_id * 10 + (data[i] - '0');
    }

    i++; // skip newline

    // add an edge to memory
    edges[line] = make_tuple(src_id, src_type,
                             dst_id, dst_type,
                             e_type, graph_id);
    line++;
  }

  close(fd);

#ifdef VERBOSE
    for (uint32_t i = 0; i < edges.size(); i++) {
      cout << "Edge " << i << ": ";
      print_edge(edges[i]);
      cout << endl;
    }
#endif
}

}
