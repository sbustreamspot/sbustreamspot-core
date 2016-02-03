#ifndef SWOOSH_STREAMHASH_H_
#define SWOOSH_STREAMHASH_H_

#include <bitset>
#include "param.h"

namespace std {

double streamhash_similarity(const bitset<L>& sketch1, const bitset<L>& sketch2);

}
#endif
