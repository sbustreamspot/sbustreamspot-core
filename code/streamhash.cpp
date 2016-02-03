#include <bitset>
#include "param.h"
#include "streamhash.h"

namespace std {

double streamhash_similarity(const bitset<L>& sketch1, const bitset<L>& sketch2) {
  // XOR = 0 if bits are the same, 1 otherwise
  return static_cast<double>((~(sketch1 ^ sketch2)).count()) / L;
}

}
