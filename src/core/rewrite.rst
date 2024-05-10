Rewrite
=============

**Header:** ``mockturtle/algorithms/cut_rewriting.hpp``

The following example shows how to rewrite an MIG using precomputed optimum networks.  In this case the maximum number of variables for a node function is 4.

It is possible to change the cost function of nodes in cut rewriting.  Here is
an example, in which the cost function only accounts for AND gates in a network,
which corresponds to the multiplicative complexity of a function.

.. code-block:: c++

   template<class Ntk>
   struct mc_cost
   {
     uint32_t operator()( Ntk const& ntk, node<Ntk> const& n ) const
     {
       return ntk.is_and( n ) ? 1 : 0;
     }
   };

   SomeResynthesisClass resyn;
   ntk = cut_rewriting<SomeResynthesisClass, mc_cost>( ntk, resyn );

Parameters and statistics
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: mockturtle::cut_rewriting_params
   :members:

.. doxygenstruct:: mockturtle::cut_rewriting_stats
   :members:

Algorithm
~~~~~~~~~

.. doxygenfunction:: mockturtle::cut_rewriting
.. doxygenfunction:: mockturtle::cut_rewriting_with_compatibility_graph

Rewriting functions
~~~~~~~~~~~~~~~~~~~

One can use resynthesis functions that can be passed to `node_resynthesis`, see
:ref:`node_resynthesis_functions`.
