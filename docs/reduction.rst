Reduction
=============

**Header:** ``mockturtle/algorithms/functional_reduction.hpp``

The following example shows how to perform functional reduction
to remove constant nodes and functionally equivalent nodes in
the network.

.. code-block:: c++

   /* derive some AIG */
   aig_network aig = ...;

   functional_reduction( aig );