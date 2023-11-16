/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/*
  \file stp_npn.hpp
  \brief Replace with size-optimum STP-based exact synthesis from NPN

  \author Homyoung
*/

#pragma once

#include <iostream>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/npn.hpp>
#include <kitty/print.hpp>
#include <mockturtle/mockturtle.hpp>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "../../core/misc.hpp"
#include "stp_cleanup.hpp"

namespace mockturtle {

class stp_npn_resynthesis {
 public:
  /*! \brief Default constructor.
   *
   */
  stp_npn_resynthesis() { build_db(); }

  template <typename LeavesIterator, typename Fn>
  void operator()(klut_network& klut,
                  kitty::dynamic_truth_table const& function,
                  LeavesIterator begin, LeavesIterator end, Fn&& fn) const {
    assert(function.num_vars() <= 4);
    const auto fe = kitty::extend_to(function, 4);
    const auto config = kitty::exact_npn_canonization(fe);

    auto func_str = "0x" + kitty::to_hex(std::get<0>(config));
    const auto it = class2signal.find(func_str);
    assert(it != class2signal.end());

    std::vector<klut_network::signal> pis(4, klut.get_constant(false));
    std::copy(begin, end, pis.begin());

    std::vector<klut_network::signal> pis_perm(4);
    auto perm = std::get<2>(config);
    for (auto i = 0; i < 4; ++i) {
      pis_perm[i] = pis[perm[i]];
    }

    // const auto& phase = std::get<1>(config);
    // for (auto i = 0; i < 4; ++i) {
    //   if ((phase >> perm[i]) & 1) {
    //     pis_perm[i] = !pis_perm[i];
    //   }
    // }

    for (auto const& po : it->second) {
      topo_view topo{db, po};
      auto f =
          stp_cleanup_dangling(topo, klut, pis_perm.begin(), pis_perm.end())
              .front();

      if (!fn(f)) return; /* quit */
    }
  }

 private:
  std::unordered_map<std::string, std::vector<std::string>> opt_klut;

  void load_optimal_klut() {
    std::ifstream infile("../src/networks/stp/opt_map.txt");
    if (!infile) {
      std::cout << " Cannot open file " << std::endl;
      assert(false);
    }

    std::string line;
    std::vector<std::string> v;
    while (std::getline(infile, line)) {
      v.clear();
      auto strs = phyLS::split_by_delim(line, ' ');
      for (auto i = 1u; i < strs.size(); i++) v.push_back(strs[i]);
      opt_klut.insert(std::make_pair(strs[0], v));
    }
  }

  std::vector<klut_network::signal> create_klut_from_str_vec(
      const std::vector<std::string> strs,
      const std::vector<klut_network::signal>& signals) {
    auto sig = signals;
    std::vector<klut_network::signal> result;

    int a, b;
    for (auto i = 0; i < strs.size(); i++) {
      const auto substrs = phyLS::split_by_delim(strs[i], '-');
      assert(substrs.size() == 4u);

      a = std::stoi(substrs[2]);
      b = std::stoi(substrs[3]);
      switch (std::stoi(substrs[1])) {
        case 0: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 15));
          break;
        }

        case 1: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 14));
          break;
        }

        case 2: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 13));
          break;
        }

        case 3: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 12));
          break;
        }

        case 4: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 11));
          break;
        }

        case 5: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 10));
          break;
        }

        case 6: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 9));
          break;
        }

        case 7: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 8));
          break;
        }

        case 8: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 7));
          break;
        }

        case 9: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 6));
          break;
        }

        case 10: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 5));
          break;
        }

        case 11: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 4));
          break;
        }

        case 12: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 3));
          break;
        }

        case 13: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 2));
          break;
        }

        case 14: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 1));
          break;
        }

        case 15: {
          sig.push_back(db._create_node({sig[a], sig[b]}, 0));
          break;
        }

        default:
          assert(false);
          break;
      }
    }

    const auto driver = sig[sig.size() - 1];
    db.create_po(driver);
    result.push_back(driver);
    return result;
  }

  void build_db() {
    std::vector<klut_network::signal> signals;
    signals.push_back(db.get_constant(false));

    for (auto i = 0u; i < 4; ++i) signals.push_back(db.create_pi());

    load_optimal_klut();

    for (const auto e : opt_klut) {
      class2signal.insert(
          std::make_pair(e.first, create_klut_from_str_vec(e.second, signals)));
    }
  }

  klut_network db;
  std::unordered_map<std::string, std::vector<klut_network::signal>>
      class2signal;
};

} /* namespace mockturtle */
