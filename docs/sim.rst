Sim
=============

**Header:** ``mockturtle/algorithms/simulation.hpp``

Simulates a network with a generic simulator.
This is a generic simulation algorithm that can simulate arbitrary values. 

The following simulators are implemented:

* ``mockturtle::default_simulator<bool>``: This simulator simulates Boolean
  values.  A vector with assignments for each primary input must be passed to
  the constructor.
* ``mockturtle::default_simulator<kitty::static_truth_table<NumVars>>``: This
  simulator simulates truth tables.  Each primary input is assigned the
  projection function according to the index.  The number of variables must be
  known at compile time.
* ``mockturtle::default_simulator<kitty::dynamic_truth_table>``: This simulator
  simulates truth tables.  Each primary input is assigned the projection
  function according to the index.  The number of variables be passed to the
  constructor of the simulator.
* ``mockturtle::partial_simulator``: This simulator simulates partial truth tables,
  whose length is flexible and new simulation patterns can be added.