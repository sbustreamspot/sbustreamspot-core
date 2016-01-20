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
#ifdef DEBUG
    cout << "Reading edges from: " << filename << endl;
#endif

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

}
