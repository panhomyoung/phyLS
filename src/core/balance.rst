Balance
=============

**Header:** ``mockturtle/algorithms/balancing.hpp``

Parameters
~~~~~~~~~~

.. doxygenstruct:: mockturtle::aig_balancing_params
   :members:

Algorithm
~~~~~~~~~

.. doxygenfunction:: mockturtle::aig_balance

Balancing of a logic network

This function implements a dynamic-programming and cut-enumeration based balancing algorithm. 
It returns a new network of the same type and performs generic balancing by providing a rebalancing function.