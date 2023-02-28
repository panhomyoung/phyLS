Cec
=============

**Header:** ``mockturtle/algorithms/equivalence_checking.hpp``

Combinational equivalence checking.

This function expects as input a miter circuit that can be generated, e.g., with the function miter. 
It returns an optional which is nullopt, if no solution can be found, this happens when a resource limit is set using the function parameters. 
Otherwise it returns true, if the miter is equivalent or false, if the miter is not equivalent. 
In the latter case the counter example is written to the statistics pointer as a std::vector<bool> following the same order as the primary inputs.