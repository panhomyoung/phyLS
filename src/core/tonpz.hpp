/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2024 */

/**
 * @file to_npz.hpp
 *
 * @brief transforms the current network into adjacency matrix(.npz)
 *
 * @author Homyoung
 * @since  2024/03/01
 */

#pragma once

#include <fmt/format.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <sstream>
#include <string>

using namespace std;  // 名字空间

namespace phyLS {

template <class Ntk>
void write_npz(Ntk const& ntk, std::ostream& os) {
  std::stringstream connect;
  cout << "num_pis: " << ntk.num_pis() << endl;
  cout << "num_pos: " << ntk.num_pos() << endl;
  cout << "num_gates: " << ntk.num_gates() << endl;
  int num_pis = ntk.num_pis(), num_pos = ntk.num_pos(),
      num_gates = ntk.num_gates();
  int size = num_pis + num_pos + num_gates;
  vector<vector<double>> adj(size, vector<double>(size, 0));

  ntk.foreach_node([&](auto const& n) {
    if (ntk.is_constant(n) || ntk.is_ci(n)) return true;

    ntk.foreach_fanin(n, [&](auto const& f) {
      if (ntk.node_to_index(ntk.get_node(f)) <= num_pis) {
        adj[ntk.node_to_index(n) - num_pis - 1]
           [ntk.node_to_index(ntk.get_node(f)) + num_gates - 1] = 1;
        adj[ntk.node_to_index(ntk.get_node(f)) + num_gates - 1]
           [ntk.node_to_index(n) - num_pis - 1] = 1;
      } else {
        adj[ntk.node_to_index(n) - num_pis - 1]
           [ntk.node_to_index(ntk.get_node(f)) - num_pis - 1] = 0.5;
        adj[ntk.node_to_index(ntk.get_node(f)) - num_pis - 1]
           [ntk.node_to_index(n) - num_pis - 1] = 0.5;
      }
      return true;
    });
    return true;
  });

  ntk.foreach_po([&](auto const& f, auto i) {
    if (ntk.node_to_index(ntk.get_node(f)) <= num_pis) {
      adj[i + num_pis + num_gates]
         [ntk.node_to_index(ntk.get_node(f)) + num_gates - 1] = 1;
      adj[ntk.node_to_index(ntk.get_node(f)) + num_gates - 1]
         [i + num_pis + num_gates] = 1;
    } else {
      adj[i + num_pis + num_gates]
         [ntk.node_to_index(ntk.get_node(f)) - num_pis - 1] = 0.5;
      adj[ntk.node_to_index(ntk.get_node(f)) - num_pis - 1]
         [i + num_pis + num_gates] = 0.5;
    }
  });

  for (auto x : adj) {
    for (int i = 0; i < x.size(); i++) {
      if (i == x.size() - 1) {
        connect << fmt::format("{}", x[i]);
      } else {
        connect << fmt::format("{}", x[i]) << ",";
      }
    }
    connect << "\n";
  }
  os << connect.str();
}

template <class Ntk>
void write_def(Ntk const& ntk, std::ostream& os_def, std::ostream& os_mdef) {
  std::stringstream components, pins, pins_def;
  cout << "num_pis: " << ntk.num_pis() << endl;
  cout << "num_pos: " << ntk.num_pos() << endl;
  cout << "num_gates: " << ntk.num_gates() << endl;
  int num_pis = ntk.num_pis(), num_pos = ntk.num_pos(),
      num_gates = ntk.num_gates();
  int size = num_pis + num_pos + num_gates;
  int output_position = 0, input_position = 0, max_position = 10 * size;

  components << fmt::format("COMPONENTS {} ;\n", num_gates);

  ntk.foreach_node([&](auto const& n) {
    if (ntk.is_constant(n) || ntk.is_ci(n)) return true;

    components << fmt::format("   - and_{}_ and\n", ntk.node_to_index(n))
               << "      + UNPLACED ;\n";
    return true;
  });
  components << "END COMPONENTS\n"
             << "\n";

  pins << fmt::format("PINS {} ;\n", num_pis + num_pos + 1);
  pins_def << fmt::format("PINS {} ;\n", num_pis + num_pos + 1);

  ntk.foreach_po([&](auto const& f, auto i) {
    pins << fmt::format("   - output_{} + NET output_{}\n", i, i)
         << "      + DIRECTION OUTPUT\n"
         << fmt::format("      + PLACED ( {} {} ) N\n", output_position * 10,
                        max_position)
         << "      + LAYER metal3 ( 0 0 ) ( 510 100 ) ;\n";
    pins_def << fmt::format("   - output_{} + NET output_{}\n", i, i)
             << "      + DIRECTION OUTPUT\n"
             << fmt::format("      + PLACED ( {} {} ) N\n",
                            output_position * 10, max_position)
             << "      + LAYER metal3 ( 0 0 ) ( 510 100 ) ;\n";
    output_position++;
  });

  ntk.foreach_pi([&](auto const& f, auto i) {
    pins << fmt::format("   - input_{} + NET input_{}\n", i, i)
         << "      + DIRECTION INPUT\n"
         << fmt::format("      + PLACED ( {} {} ) N\n", input_position * 10, 0)
         << "      + LAYER metal3 ( 0 0 ) ( 510 100 ) ;\n";
    pins_def << fmt::format("   - input_{} + NET input_{}\n", i, i)
             << "      + DIRECTION INPUT\n"
             << fmt::format("      + PLACED ( {} {} ) N\n", input_position * 10,
                            0)
             << "      + LAYER metal3 ( 0 0 ) ( 510 100 ) ;\n";
    input_position++;
  });
  pins << "END PINS\n";
  pins_def << "    - input_clk + NET input_clk\n"
           << "      + DIRECTION INPUT\n"
           << fmt::format("      + PLACED ( {} {} ) N\n", input_position * 10,
                          0)
           << "      + LAYER metal3 ( 0 0 ) ( 510 100 ) ;\n";
  pins_def << "END PINS\n";

  os_mdef << components.str() << pins.str();
  os_def << components.str() << pins_def.str();
}

template <class Ntk>
void write_npz(Ntk const& ntk, std::string const& filename) {
  std::ofstream os(filename.c_str(), std::ofstream::out);
  write_npz(ntk, os);
  os.close();
}

template <class Ntk>
void write_def(Ntk const& ntk, std::string const& filename_def,
               std::string const& filename_mdef) {
  std::ofstream os_def(filename_def.c_str(), std::ofstream::out);
  std::ofstream os_mdef(filename_mdef.c_str(), std::ofstream::out);
  write_def(ntk, os_def, os_mdef);
  os_def.close();
  os_mdef.close();
}

}  // namespace phyLS