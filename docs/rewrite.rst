Rewrite
=============

**Header:** ``mockturtle/algorithms/mig_algebraic_rewriting.hpp``

**Header:** ``mockturtle/algorithms/xag_algebraic_rewriting.hpp``


Majority algebraic depth rewriting.

This algorithm tries to rewrite a network with majority gates for depth optimization using the associativity and distributivity rule in majority-of-3 logic. 
It can be applied to networks other than MIGs, but only considers pairs of nodes which both implement the majority-of-3 function.