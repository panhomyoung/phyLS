Examples
============

Synthesis of EPFL benchmarks
----------------------------------

In the following example, we show how `phyLS` can be used to synthesize a EPFL benchamrk. 

.. .. code-black:: c++

..     spec spec;
..     spec.verbosity = 0;

..     chain c;

..     dynamic_truth_table x( 3 ), y( 3 ), z( 3 );

..     create_nth_var( x, 0 );
..     create_nth_var( y, 1 );
..     create_nth_var( z, 2 );

..     // The sum and carry functions represent the outputs of the
..     // chain that we want to synthesize.
..     auto const sum = x ^ y ^ z;
..     auto const carry = ternary_majority( x, y, z );
..     spec[0] = sum;
..     spec[1] = carry;

..     // Call the synthesizer with the specification we've constructed.
..     auto const result = synthesize( spec, c );

..     // Ensure that synthesis was successful.
..     assert( result == success );

..     // Simulate the synthesized circuit and ensure that it
..     // computes the correct functions.
..     auto sim_fs = c.simulate();
..     assert( sim_fs[0] == sum );
..     assert( sim_fs[1] == carry );

.. In this example, we synthesize a Boolean chain for a full adder
.. specified by the two Boolean functions `sum` and `carry`.  We see how
.. synthesis is invoked using the `synthesize` function that takes two
.. parameters.  The first parameter is the specification `spec`, the
.. second parameter `c` references a chain.  If synthesis is successful,
.. the `synthesize` function returns `success` and stores the synthesized
.. chain in `c`.  Last but not least, we simulate the chain to ensure
.. that it's output functions are equivalent to the specified functions
.. of the full adder.

.. Percy offers several different encodings and synthesis methods, and
.. allows its users to select from various SAT solver backends.  By
.. default all engines use ABC's `bsat` solver backend [1]_
.. (`SLV_BSAT2`), the SSV encoding (`ENC_SSV`), and the standard
.. synthesis method (`SYNTH_STD`).  Suppose that this particular
.. combination is not suitable for our workflow.  We can then easily
.. customize the synthesis process by cherry-picking a solver, encoder,
.. and synthesis method from the available options.

.. The next example demonstrates fence-based synthesis using the
.. corresponding encoder and synthesis method together with ABC's `bsat`
.. as solver backend:

.. .. code-black:: c++

..     percy::SolverType solver_type = percy::SLV_BSAT2;
..     percy::EncoderType encoder_type = percy::ENC_FENCE;
..     percy::SynthMethod synth_method = percy::SYNTH_FENCE;

..     auto solver = get_solver( solver_type );
..     auto encoder = get_encoder( *solver, encoder_type );
..     auto const result = synthesize( spec, c, *solver, *encoder, synth_method );

.. Enumerate (and count) partial DAGs
.. ----------------------------------

.. In the following code snippet, we use `percy` to enumerate partial
.. DAGs for a given number of nodes (up to 7 nodes), count them, and
.. print the numbers.

.. .. code-black:: c++

..   #include <percy/partial_dag.hpp>

..   for ( auto i = 1u; i < 8; ++i )
..   {
..     const auto dags = percy::pd_generate( i );
..     std::cout << i << ' ' << dags.size() << std::endl;
..   }


.. .. [1] https://github.com/berkeley-abc/abc
