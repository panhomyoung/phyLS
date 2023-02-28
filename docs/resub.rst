Resub
=============

**Header:** ``mockturtle/algorithms/resubstitution.hpp``

Several resubstitution algorithms are implemented and can be called directly, including:

- ``default_resubstitution`` does functional reduction within a window.

- ``aig_resubstitution``, ``mig_resubstitution`` and ``xmg_resubstitution`` do window-based resubstitution in the corresponding network types.

- ``resubstitution_minmc_withDC`` minimizes multiplicative complexity in XAGs with window-based resubstitution.

- ``sim_resubstitution`` does simulation-guided resubstitution in AIGs or XAGs.