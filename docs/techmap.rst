Techmap
=============

**Header:** ``mockturtle/algorithms/mapper.hpp``

A versatile mapper that supports technology mapping and graph mapping
(optimized network conversion). The mapper is independent of the
underlying graph representation. Hence, it supports generic subject
graph representations (e.g., AIG, and MIG) and a generic target
representation (e.g. cell library, XMG). The mapper aims at finding a
good mapping with respect to delay, area, and switching power.

The mapper uses a library (hash table) to facilitate Boolean matching.
For technology mapping, it needs `tech_library` while for graph mapping
it needs `exact_library`. For technology mapping, the generation of both NP- and
P-configurations of gates are supported. Generally, it is convenient to use
NP-configurations for small or medium size cell libraries. For bigger libraries,
P-configurations should perform better. You can test both the configurations to
see which one has the best run time. For graph mapping, NPN classification
is used instead.