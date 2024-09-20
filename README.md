# ELMap: k-LUT Network Exact Synthesis based Area-Driven LUT Mapping

ELMap is based on the [mockturtle](https://github.com/lsils/mockturtle) and the [abc](https://github.com/berkeley-abc/abc), it can optimize different logics attributes. 
Currently, it supports mockturtle format(AIG, MIG, XAG, XMG) and abc format(AIG,GIA) based optimization.

## Requirements
A modern compiler is required to build the libraries. 
Compiled successfully with Clang 6.0.1, Clang 12.0.0, GCC 7.3.0, and GCC 8.2.0. 

## How to Compile
```bash
git clone --recursive https://anonymous.4open.science/r/ELMap
cd phyLS
mkdir build
cd build
cmake ..
make
./bin/phyLS
```
