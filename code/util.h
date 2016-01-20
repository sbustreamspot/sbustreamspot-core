#ifndef SWOOSH_UTIL_H_
#define SWOOSH_UTIL_H_

#include <string>
#include <iostream>

namespace std {

inline void panic(string message) {
  cout << message << endl;
  exit(-1);
}


}

#endif
