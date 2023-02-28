Lut_mapping
=============

**Header:** ``mockturtle/algorithms/collapse_mapped.hpp``

Collapse mapped network into k-LUT network.

Collapses a mapped network into a k-LUT network. In the mapped network each cell is represented in terms of a collection of nodes from the subject graph. This method creates a new network in which each cell is represented by a single node.

This function performs some optimizations with respect to possible output complementations in the subject graph:

If an output driver is only used in positive form, nothing changes

If an output driver is only used in complemented form, the cell function of the node is negated.

If an output driver is used in both forms, two nodes will be created for the mapped node.