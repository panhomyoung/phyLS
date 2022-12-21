# powerful heightened yielded Logic Synthesis (phyLS)

phyLS is based on the [mockturtle](https://github.com/lsils/mockturtle), it can optimize different logics attributes. 
Currently, it supports AIG, MIG, XAG, and XMG based optimization.

[Read the documentation here.](https://phyls.readthedocs.io/en/latest/)

## Requirements
A modern compiler is required to build the libraries. 
Compiled successfully with Clang 6.0.1, Clang 12.0.0, GCC 7.3.0, and GCC 8.2.0. 

## How to Compile
```bash
git clone --recursive https://github.com/panhongyang0/phyLS.git
cd phyLS
mkdir build
cd build
cmake ..
make
./bin/phyLS
```
