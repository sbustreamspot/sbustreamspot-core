#!/usr/bin/env python

import sys
from mpl_toolkits.mplot3d import *
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import cm

train_frac = float(sys.argv[1])

xs = set([])
ys = set([])
zs_m = {}
zs_s = {}
with open('silhouette_' + str(int(train_frac * 100)) + '.txt', 'r') as f:
    for line in f:
        fields = line.strip().split('\t')
        C = int(fields[0])
        k = int(fields[1])
        m = float(fields[2])
        s = float(fields[3])
        
        xs.add(C)
        ys.add(k)
        zs_m[C,k] = m
        zs_s[C,k] = s

xs = list(sorted(xs))
ys = list(sorted(ys))
x_surf, y_surf = np.meshgrid(xs, ys)
z_surf = np.zeros((len(ys), len(xs)))

for i in range(len(ys)):
    for j in range(len(xs)):
        z_surf[i][j] = zs_m[x_surf[i][j], y_surf[i][j]]

print x_surf
print y_surf
print z_surf

fig = plt.figure()
plt.hold(True)
ax = fig.gca(projection='3d')
ax.plot_surface(x_surf, y_surf, z_surf,
                rstride=1, cstride=1, cmap=cm.hot, alpha=0.7)
ax.set_xlabel('Chunk Length')
ax.set_ylabel('Number of Clusters')
ax.set_zlabel('Silhouette Coefficient')
plt.savefig('silhouette3d_' + str(int(train_frac * 100)) + '.pdf',
            bbox_inches='tight')
plt.clf()
plt.close()

fig = plt.figure()
plt.hold(True)
ax = fig.gca()
ax.pcolor(z_surf, cmap=plt.cm.Blues)
ax.set_xticks(np.arange(len(xs))+0.5, minor=False)
ax.set_yticks(np.arange(len(ys))+0.5, minor=False)
ax.set_xticklabels(xs, minor=False)
ax.set_yticklabels(ys, minor=False)
ax.set_xlabel('Chunk Length')
ax.set_ylabel('Number of Clusters')
plt.savefig('silhouette_' + str(int(train_frac * 100)) + '.pdf',
            bbox_inches='tight')
