Refactor
=============

**Header:** ``mockturtle/algorithms/refactoring.hpp``

It is possible to change the cost function of nodes in refactoring.  Here is
an example, in which the cost function does not account for XOR gates in a network. 
This may be helpful in logic synthesis addressing cryptography applications where
XOR gates are considered "for free". 