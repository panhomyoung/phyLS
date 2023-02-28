Write into file formats
-----------------------

Write into AIGER files
~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_aiger.hpp``

Writes a combinational AIG network in binary AIGER format into a file.

This function should be only called on “clean” aig_networks.

Write into BENCH files
~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_bench.hpp``

Writes network in BENCH format into a file.

Write into BLIF files
~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_blif.hpp``

Writes network in BLIF format into a file.

Write into structural Verilog files
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_verilog.hpp``

Writes network in structural Verilog format into a file.

Write into DIMACS files (CNF)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_cnf.hpp``

Writes network into CNF DIMACS format.

It also adds unit clauses for the outputs. Therefore a satisfying solution is one that makes all outputs 1.

.. _write_dot:

Write into DOT files (Graphviz)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_dot.hpp``

Writes network in DOT format into a file.
