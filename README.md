# phyLS (powerful )

This is 

The SAT part is same as normal SAT solver.

The implementation of the AllSAT part differs from incremental enumeration because we do not add blocking conditions for existing solutions, but rather compute the matrices to obtain all the solutions in one pass.

## Requirements
A modern compiler is required to build the libraries. Compiled successfully with Clang 6.0.1, Clang 12.0.0, GCC 7.3.0, and GCC 8.2.0. 

## How to Compile
```bash
git clone --recursive https://github.com/panhongyang0/phySAT.git
cd phySAT
mkdir build
cd build
cmake ..
make
./bin/phySAT
```
## Useage
```bash
help  // all useful commands
1. cnf -f xx.cnf // CNF-based AllSAT solver, the benchmark is in <benchmark> 
```
