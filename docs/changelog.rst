Change Log
==========

Next release
------------

v2.0 (August 03, 2023)
------------------------

* Semi-tensor product (STP) based k-LUT network simulation ``simulator``, which is faster than ``sim``
* k-LUT network mapping ``lut_mapping``, default 4-LUT
* NPN based network Logic synthesis ``rewrite``, faster and more efficient
* STP-based functional reduction ``stpfr``
* Logic synthesis commands ``fr``

* Exact synthesis to find optimal 2-LUTs commands ``exact``
* 2-LUT rewriting commands ``lutrw``, which enable technology dependent rewriting

* ABC Logic synthesis commands ``aresub``
* ABC Logic synthesis commands ``fraig``
* ABC Logic synthesis commands ``strash``
* ABC Logic synthesis commands ``comb``
* ABC Logic synthesis commands ``acec``
* ABC GIA Logic synthesis commands ``Afraig``
* ABC GIA Logic synthesis commands ``Aget``

* convert store element into ABC store ``convert``, which implement conversion between different data structures

v1.0 (December 20, 2022)
------------------------

* Initial release
* Data structures ``aiger``, ``load``, ``bench``, ``verilog``, ``blif``
* Logic synthesis commands ``balance``
* Logic synthesis commands ``resub``
* Logic resynthesis commands ``resyn``
* Logic synthesis commands ``rewrite``
* Logic synthesis commands ``reduction``
* Logic synthesis commands ``refactor``

* Compute truth table for expression ``exprsim``
* Combinational equivalence checking for AIG network ``cec``
* SAT solver ``sat``
* Logic network simulation ``sim``

* FPGA technology mapping of the network ``lutmap``
* Standard cell mapping ``techmap``
