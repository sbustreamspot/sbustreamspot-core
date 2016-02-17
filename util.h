/* 
 * Copyright 2016 Emaad Ahmed Manzoor
 * License: Apache License, Version 2.0
 * http://www3.cs.stonybrook.edu/~emanzoor/streamspot/
 */

#ifndef STREAMSPOT_UTIL_H_
#define STREAMSPOT_UTIL_H_

#include <string>
#include <iostream>

namespace std {

inline void panic(string message) {
  cout << message << endl;
  exit(-1);
}


}

#endif
