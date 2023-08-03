Examples
============

All commands
----------------

Input:
::
    help


Output:
::
    ABC commands:
     aresub           comb             fraig            strash

    Verification commands:
     acec             cec              exprsim          sat
     sim              simulator

    Mapping commands:
     lut_mapping      lutmap           techmap

    Synthesis commands:
     balance          create_graph     exact            fr
     lutrw            refactor         resub            resyn
     rewrite          stpfr

    I/O commands:
     load             read             read_aiger       read_bench
     read_blif        read_genlib      read_gia         read_verilog
     write            write_aiger      write_bench      write_blif
     write_dot        write_genlib     write_gia        write_verilog

    Gia commands:
     Afraig           Aget

    General commands:
     alias            convert          current          help
     print            ps               quit             set
     show             store

For more details, simply add '-h' after command to see all options of this command.

Synthesis of EPFL benchmarks
----------------

In the following example, we show how `phyLS` can be used to synthesize a EPFL benchamrk. 

Input:
::
    read_aiger ~/phyLS/benchmarks/adder.aig
    ps -a
    resub // any synthesis commands
    ps -a
    read_genlib ~/phyLS/src/mcnc.genlib
    techmap

Output:
::
    $ AIG   i/o = 256/129   gates = 1020   level = 255
    $ ntk   i/o = 256/129   gates = 893   level = 256
    [CPU time]   0.09 s
    $ Mapped AIG into #gates = 701 area = 1849.00 delay = 204.90
