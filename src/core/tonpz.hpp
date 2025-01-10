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
  //   cout << "num_pis: " << ntk.num_pis() << endl;
  //   cout << "num_pos: " << ntk.num_pos() << endl;
  //   cout << "num_gates: " << ntk.num_gates() << endl;
  int num_pis = ntk.num_pis(), num_pos = ntk.num_pos(),
      num_gates = ntk.num_gates();
  int size = num_pis + num_pos + num_gates;
  cout << "size: " << size << endl;
  vector<double> value;
  vector<vector<int>> coordinate(2);

  ntk.foreach_node([&](auto const& n) {
    if (ntk.is_constant(n) || ntk.is_ci(n)) return true;

    ntk.foreach_fanin(n, [&](auto const& f) {
      if (ntk.node_to_index(ntk.get_node(f)) <= num_pis) {
        value.push_back(0.6);
        coordinate[0].push_back(ntk.node_to_index(n) - num_pis - 1);
        coordinate[1].push_back(ntk.node_to_index(ntk.get_node(f)) + num_gates -
                                1);
        value.push_back(0.6);
        coordinate[0].push_back(ntk.node_to_index(ntk.get_node(f)) + num_gates -
                                1);
        coordinate[1].push_back(ntk.node_to_index(n) - num_pis - 1);
      } else {
        value.push_back(1);
        coordinate[0].push_back(ntk.node_to_index(n) - num_pis - 1);
        coordinate[1].push_back(ntk.node_to_index(ntk.get_node(f)) - num_pis -
                                1);
        value.push_back(1);
        coordinate[0].push_back(ntk.node_to_index(ntk.get_node(f)) - num_pis -
                                1);
        coordinate[1].push_back(ntk.node_to_index(n) - num_pis - 1);
      }
      return true;
    });
    return true;
  });

  ntk.foreach_po([&](auto const& f, auto i) {
    if (ntk.node_to_index(ntk.get_node(f)) <= num_pis) {
      value.push_back(0.1);
      coordinate[0].push_back(i + num_pis + num_gates);
      coordinate[1].push_back(ntk.node_to_index(ntk.get_node(f)) + num_gates -
                              1);
      value.push_back(0.1);
      coordinate[0].push_back(ntk.node_to_index(ntk.get_node(f)) + num_gates -
                              1);
      coordinate[1].push_back(i + num_pis + num_gates);
    } else {
      value.push_back(0.6);
      coordinate[0].push_back(i + num_pis + num_gates);
      coordinate[1].push_back(ntk.node_to_index(ntk.get_node(f)) - num_pis - 1);
      value.push_back(0.6);
      coordinate[0].push_back(ntk.node_to_index(ntk.get_node(f)) - num_pis - 1);
      coordinate[1].push_back(i + num_pis + num_gates);
    }
  });

  for (int i = 0; i < value.size(); i++) {
    if (i == value.size() - 1) {
      connect << fmt::format("{}", value[i]);
    } else {
      connect << fmt::format("{}", value[i]) << ",";
    }
  }
  connect << "\n";
  for (auto x : coordinate) {
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
  //   cout << "num_pis: " << ntk.num_pis() << endl;
  //   cout << "num_pos: " << ntk.num_pos() << endl;
  //   cout << "num_gates: " << ntk.num_gates() << endl;
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

void write_def(mockturtle::binding_view<mockturtle::klut_network> const& ntk,
               std::ostream& os_def, std::ostream& os_mdef) {
  std::stringstream components, pins, pins_def;
  //   cout << "num_pis: " << ntk.num_pis() << endl;
  //   cout << "num_pos: " << ntk.num_pos() << endl;
  //   cout << "num_gates: " << ntk.num_gates() << endl;
  int num_pis = ntk.num_pis(), num_pos = ntk.num_pos(),
      num_gates = ntk.num_gates();
  int size = num_pis + num_pos + num_gates;
  int output_position = 0, input_position = 0, max_position = 30 * size;

  components << fmt::format("COMPONENTS {} ;\n", num_gates);

  auto const& gates = ntk.get_library();

  ntk.foreach_gate([&](auto const& n) {
    if (ntk.is_constant(n) || ntk.is_ci(n)) {
      std::cout << "there is constant and ci\n";
      return true;
    }

    components << fmt::format("   - gate_{}_ {}\n", ntk.node_to_index(n),
                              gates[ntk.get_binding_index(n)].name)
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
void write_netlist(Ntk const& ntk, std::ostream& os) {
  std::stringstream connect;
  //   cout << "num_pis: " << ntk.num_pis() << endl;
  //   cout << "num_pos: " << ntk.num_pos() << endl;
  //   cout << "num_gates: " << ntk.num_gates() << endl;
  int num_pis = ntk.num_pis(), num_pos = ntk.num_pos(),
      num_gates = ntk.num_gates();
  int size = num_pis + num_pos + num_gates;
  cout << "size: " << size << endl;
  vector<double> value;
  vector<vector<int>> coordinate(2);

  ntk.foreach_node([&](auto const& n) {
    if (ntk.is_constant(n) || ntk.is_ci(n)) return true;

    ntk.foreach_fanin(n, [&](auto const& f) {
      if (ntk.node_to_index(ntk.get_node(f)) <= num_pis) {
        value.push_back(0.6);
        coordinate[0].push_back(ntk.node_to_index(n) - num_pis - 1);
        coordinate[1].push_back(ntk.node_to_index(ntk.get_node(f)) + num_gates -
                                1);
        value.push_back(0.6);
        coordinate[0].push_back(ntk.node_to_index(ntk.get_node(f)) + num_gates -
                                1);
        coordinate[1].push_back(ntk.node_to_index(n) - num_pis - 1);
      } else {
        value.push_back(1);
        coordinate[0].push_back(ntk.node_to_index(n) - num_pis - 1);
        coordinate[1].push_back(ntk.node_to_index(ntk.get_node(f)) - num_pis -
                                1);
        value.push_back(1);
        coordinate[0].push_back(ntk.node_to_index(ntk.get_node(f)) - num_pis -
                                1);
        coordinate[1].push_back(ntk.node_to_index(n) - num_pis - 1);
      }
      return true;
    });
    return true;
  });

  ntk.foreach_po([&](auto const& f, auto i) {
    if (ntk.node_to_index(ntk.get_node(f)) <= num_pis) {
      value.push_back(0.1);
      coordinate[0].push_back(i + num_pis + num_gates);
      coordinate[1].push_back(ntk.node_to_index(ntk.get_node(f)) + num_gates -
                              1);
      value.push_back(0.1);
      coordinate[0].push_back(ntk.node_to_index(ntk.get_node(f)) + num_gates -
                              1);
      coordinate[1].push_back(i + num_pis + num_gates);
    } else {
      value.push_back(0.6);
      coordinate[0].push_back(i + num_pis + num_gates);
      coordinate[1].push_back(ntk.node_to_index(ntk.get_node(f)) - num_pis - 1);
      value.push_back(0.6);
      coordinate[0].push_back(ntk.node_to_index(ntk.get_node(f)) - num_pis - 1);
      coordinate[1].push_back(i + num_pis + num_gates);
    }
  });

  for (int i = 0; i < value.size(); i++) {
    if (i == value.size() - 1) {
      connect << fmt::format("{}", value[i]);
    } else {
      connect << fmt::format("{}", value[i]) << ",";
    }
  }
  connect << "\n";
  for (auto x : coordinate) {
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

template <class Ntk>
void write_netlist(Ntk const& ntk, std::string const& filename_netlist) {
  std::ofstream os(filename_netlist.c_str(), std::ofstream::out);
  write_netlist(ntk, os);
  os.close();
}

}  // namespace phyLS