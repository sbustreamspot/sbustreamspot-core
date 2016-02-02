#!/usr/bin/env python

import matplotlib.pyplot as plt
import numpy as np
import sys

b = int(sys.argv[1])
r = int(sys.argv[2])

xs = np.arange(0., 1., 0.01)
ys = 1 - (1 - xs ** r) ** b 
threshold = (1/float(b)) ** (1/float(r))

plt.plot(xs, ys, 'b-')
plt.axvline(x=threshold, color='r', linestyle='--')
plt.xticks(list(plt.xticks()[0]) + [threshold], rotation=45)
plt.savefig('scurve.pdf', bbox_inches='tight')
