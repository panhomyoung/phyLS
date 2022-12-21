Introduction
============

The powerful heightened yielded Logic Synthesis (`phyLS`) is a growing logic synthesis tool based on EPFL Logic Synthesis Library `mockturtle` [#]_ .
It can optimize different logic attributes, such as `AIG`, `MIG`, `XAG`, and `XMG`.
`phyLS` combines logic optimization, technology mapping for look-up tables and standard cells, and verification.

`phyLS` provides an experimental implementation of these algorithms and a programming environment for building similar applications. 
It also allows users to customize `phyLS` for their needs as the `ABC` [#]_ .

`phyLS` concerns five main components of commands:

1. *General* -- This component contains some general commands based on `alice` [#]_ , such as `ps` (Print statistics), `store` (Store management), and `show` (Show store entry).
2. *I/O* -- A specialized format BAF (Binary Aig Format `.aig`) for reading/writing large AIGs into binary files. Input file also parsers for binary BLIF, BENCH format, and Verilog. Output file writers for binary BLIF, BENCH format, Verilog, and circuit graph representation DOT format. It also support truth-table as input.
3. *Synthesis* -- We lists combinational synthesis commands implemented. Fast and efficient synthesis is achieved by DAG-aware rewriting of the AIG(command `rewrite`), or collapsing and refactoring of logic cones (command `refactor`), or AIG balancing (command `balance`) to reduce the AIG size and tends to reduce the number of AIG levels, or the technology-independent restructuring of the AIG (command `resub`).
4. *Mapping* -- `phyLS` can realise both LUT-mapping (command `lutmap`) and Standard cell mapping (command `techmap`) of technology mapping.
5. *verification* -- Several equivalence checking options are currently implemented, such as combinational equivalence checking (command `cec`) and random simulation of the netowrk (command `sim`).

.. [#] https://github.com/lsils/mockturtle
.. [#] https://github.com/berkeley-abc/abc 
.. [#] https://github.com/msoeken/alice
