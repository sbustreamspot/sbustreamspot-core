import sys
import math
from graph_tool.all import *
from pylab import *  # for plotting

g = Graph()
e_type = g.new_edge_property("string")
v_id = g.new_vertex_property("int")
v_type = g.new_vertex_property("string")
v_idx = {}

with open('nodes.csv', 'r') as f:
    for idx, line in enumerate(f):
        fields = line.strip().split(',')
        id = int(fields[0])
        type = fields[1]
        
        v = g.add_vertex()
        
        v_id[v] = id
        v_type[v] = type
        v_idx[id] = idx

with open('edges.csv', 'r') as f:
    for line in f:
        fields = line.strip().split(',')
        src_id = int(fields[0])
        dst_id = int(fields[1])
        type = fields[2]

        #e = g.add_edge(v_idx[src_id], v_idx[dst_id])
        e = g.add_edge(src_id, dst_id, add_missing=True)

        e_type[e] = type

deg = g.degree_property_map("out")
degprop = g.new_vertex_property("int")
for v in g.vertices():
    if deg[v] > 0:
        print v, deg[v]
    degprop[v] = math.log(deg[v] + 1)

"""
out_hist = vertex_hist(g, "out")
y = out_hist[0]
err = sqrt(out_hist[0])
err[err >= y] = y[err >= y]

figure(figsize=(10,5))
#errorbar(out_hist[1][:-1], out_hist[0], fmt="o", yerr=err,
#                 label="out")
bar(out_hist[1][1:-1], out_hist[0][1:], align='center', width=1)
#gca().set_yscale("log")
#gca().set_xscale("log")
#gca().set_ylim(1e-1, 1e5)
#gca().set_xlim(0.8, 1e3)
#subplots_adjust(left=0.2, bottom=0.2)
tight_layout()
savefig("out-deg-dist-0.pdf")
"""

pos = random_layout(g)
graph_draw(g, pos, output_size=(15000,15000),
           #vertex_text=v_type,
           vorder=deg,
           vertex_size=degprop,
           edge_text=e_type,
           edge_pen_width=0.5,
           output='graph0.pdf')
