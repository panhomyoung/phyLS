Installation
============

If you want to run `phyLS`, you'll need to build some binaries. A modern compiler is required to build the libraries. The build instructions depend on your operating system. We are now only support Unix-like operating systems, compiled only successfully with Clang 6.0.1, Clang 12.0.0, GCC 7.3.0, and GCC 8.2.0.

Once those requirements are met, run the following commands to build and run the `phyLS`:

.. code-block:: bash

    git clone --recursive https://github.com/panhongyang0/phyLS.git
    cd phyLS
    mkdir build
    cd build
    cmake ..
    make
    ./bin/phyLS

