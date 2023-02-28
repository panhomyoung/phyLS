Lutmap
=============

**Header:** ``mockturtle/algorithms/lut_mapper.hpp``

LUT mapping with cut size :math:`k` partitions a logic network into mapped
nodes and unmapped nodes, where a mapped node :math:`n` is assigned some cut
:math:`(n, L)` such that the following conditions hold: i) each node that
drives a primary output is mapped, and ii) each leaf in the cut of a mapped
node is either a primary input or also mapped.  This ensures that the mapping
covers the whole logic network.  LUT mapping aims to find a good mapping with
respect to a cost function, e.g., a short critical path in the mapping or a
small number of mapped nodes.  The LUT mapping algorithm can assign a weight
to a cut for alternative cost functions.

This implementation offers delay- or area-oriented mapping. The mapper can be
configured using many options.