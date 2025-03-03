/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2022  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file mapper.hpp
  \brief Mapper

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <mockturtle/algorithms/mapper.hpp>
#include "MCTS.hpp"
#include "../core/utils/data_structure.hpp"

namespace mockturtle {

namespace detail {
template <class Ntk, unsigned CutSize, typename CutData, unsigned NInputs,
          classification_type Configuration>
class tech_incre_map_impl {
 public:
  using network_cuts_t = fast_network_cuts<Ntk, CutSize, true, CutData>;
  using cut_t = typename network_cuts_t::cut_t;
  using match_map =
      std::unordered_map<uint32_t, std::vector<cut_match_tech<NInputs>>>;
  using klut_map =
      std::unordered_map<uint32_t, std::array<signal<klut_network>, 2>>;
  using map_ntk_t = binding_view<klut_network>;
  using seq_map_ntk_t = binding_view<sequential<klut_network>>;

  explicit tech_incre_map_impl(
      Ntk const& ntk, tech_library<NInputs, Configuration> const& library,
      map_params const& ps, map_stats& st)
      : ntk(ntk),
        library(library),
        ps(ps),
        st(st),
        node_match(ntk.size()),
        match_position(ntk.size()),
        matches(),
        switch_activity(
            ps.eswp_rounds
                ? switching_activity(ntk, ps.switching_activity_patterns)
                : std::vector<float>(0)),
        cuts(fast_cut_enumeration<Ntk, CutSize, true, CutData>(
            ntk, ps.cut_enumeration_ps, &st.cut_enumeration_st))
            {
    std::tie(lib_inv_area, lib_inv_delay, lib_inv_id) =
        library.get_inverter_info();
    std::tie(lib_buf_area, lib_buf_delay, lib_buf_id) =
        library.get_buffer_info();
  }

  explicit tech_incre_map_impl(
      Ntk const& ntk, tech_library<NInputs, Configuration> const& library,
      std::vector<float> const& switch_activity, map_params const& ps,
      map_stats& st)
      : ntk(ntk),
        library(library),
        ps(ps),
        st(st),
        node_match(ntk.size()),
        matches(),
        switch_activity(switch_activity),
        cuts(fast_cut_enumeration<Ntk, NInputs, true, CutData>(
            ntk, ps.cut_enumeration_ps, &st.cut_enumeration_st)),
        match_position(ntk.size()) {
    std::tie(lib_inv_area, lib_inv_delay, lib_inv_id) =
        library.get_inverter_info();
    std::tie(lib_buf_area, lib_buf_delay, lib_buf_id) =
        library.get_buffer_info();
  }

  explicit tech_incre_map_impl(
      Ntk const& ntk, std::string lib_file,
      tech_library<NInputs, Configuration> const& library,
      tech_library<NInputs, Configuration> const& cri_lib,
      std::vector<node_position> const& np, map_params const& ps, map_stats& st)
      : ntk(ntk),
        lib_file(lib_file),
        library(library),
        cri_lib(cri_lib),
        np(np),
        ps(ps),
        st(st),
        node_match(ntk.size()),
        matches(),
        switch_activity(
            ps.eswp_rounds
                ? switching_activity(ntk, ps.switching_activity_patterns)
                : std::vector<float>(0)),
        cuts(fast_cut_enumeration<Ntk, CutSize, true, CutData>(
            ntk, ps.cut_enumeration_ps, &st.cut_enumeration_st)),
        cuts_unmapped(fast_cut_enumeration<Ntk, CutSize, true, CutData>(
            ntk, ps.cut_enumeration_ps, &st.cut_enumeration_st)),
        match_position(ntk.size())
  {
    std::tie(lib_inv_area, lib_inv_delay, lib_inv_id) =
        library.get_inverter_info();
    std::tie(lib_buf_area, lib_buf_delay, lib_buf_id) =
        library.get_buffer_info();
  }

  template <typename node_name>
  void get_position(binding_view<klut_network> binding_network,
                    std::vector<node_position> gate_positions, node_name nm) {
    uint32_t index = 0;
    std::vector<bool> test_vec(gate_positions.size(), false);
    for (auto& gate_position : gate_positions) {
      // actually the eventual return is index itself, ncp is a node-cut pair
      auto& ncp = res2ntk[binding_network.make_signal(
          binding_network.index_to_node(index))];
      np[ncp.node_index] = gate_position;
      std::cout << "position of " << int(ncp.node_index)
                << " is x: " << gate_position.x_coordinate
                << " y: " << gate_position.y_coordinate << "\n";
      index++;
    }
  }

  double get_reward()
  {
    return _reward;
  }

  double get_mulReward()
  {
    return _mulReward;
  }

  std::string get_state()
  {
    std::stringstream state{};
    state << fmt::format(" Delay = {:>12.2f}  Area = {:>12.2f} %\n",
            delay, area);
    return state.str();
  }
private:

#pragma region execute mapping
  bool execute_mapping() {
    /* compute mapping for delay */
    if (!ps.skip_delay_round) {
      if (!compute_mapping<false>()) {
        return false;
      }
    }
    /* compute mapping using global area flow */
    while (iteration < ps.area_flow_rounds + 1) {
      compute_required_time();
      if (!compute_mapping<true>()) {
        return false;
      }
    }

    /* compute mapping using exact area */
    while (iteration < ps.ela_rounds + ps.area_flow_rounds + 1) {
      compute_required_time();
      if (!compute_mapping_exact<false>()) {
        return false;
      }
    }

    /* compute mapping using exact switching activity estimation */
    while (iteration <
           ps.eswp_rounds + ps.ela_rounds + ps.area_flow_rounds + 1) {
      compute_required_time();
      if (!compute_mapping_exact<true>()) {
        return false;
      }
    }

    return true;
  }

  bool execute_wirelength_mapping() {
    /* compute mapping for delay */
    if (!ps.skip_delay_round) {
      if (!compute_mapping<false>()) return false;
    }

    /* compute mapping using global area flow */
    while (iteration < ps.area_flow_rounds + 1) {
      compute_required_time();
      if (!compute_mapping<true>()) return false;
    }

    /* compute mapping for wirelength */
    if (ps.wirelength_rounds) {
      compute_required_time();
      if (!compute_mapping_wirelength<false, true>()) return false;
    }

    /* compute mapping using exact area */
    while (iteration < ps.ela_rounds + ps.area_flow_rounds +
                           ps.total_wirelength_rounds + 1) {
      compute_required_time();
      if (!compute_mapping_exact<false>()) return false;
    }

    return true;
  }

  bool execute_wirelength_balance_mapping() {
    /* compute mapping for delay */
    if (!ps.skip_delay_round) {
      if (!compute_mapping<false>()) return false;
    }

    /* compute mapping using global area flow */
    while (iteration < ps.area_flow_rounds + 1) {
      compute_required_time();
      if (!compute_mapping<true>()) return false;
    }

    /* compute mapping for wirelength */
    if (ps.wirelength_rounds) {
      compute_required_time();
      if (!compute_mapping_wirelength<false, true>()) return false;
    }

    /* compute mapping for total wirelength */
    while (iteration < ps.area_flow_rounds + ps.total_wirelength_rounds + 2) {
      compute_required_time();
      if (!compute_mapping_wirelength<true, false>()) return false;
    }

    /* compute mapping using global area flow */
    while (iteration < ps.area_flow_rounds + ps.total_wirelength_rounds + 3) {
      compute_required_time();
      if (!compute_mapping<true>()) return false;
    }

    /* compute mapping using exact area */
    while (iteration < ps.ela_rounds + ps.area_flow_rounds +
                           ps.total_wirelength_rounds + 3) {
      compute_required_time();
      if (!compute_mapping_exact<false>()) return false;
    }

    return true;
  }

  bool execute_delay_mapping() {
    /* compute mapping for delay */
    if (!ps.skip_delay_round) {
      if (!compute_mapping<false>()) {
        return false;
      }
    }

    /* compute mapping using global area flow */
    while (iteration < ps.area_flow_rounds + 1) {
      compute_required_time();
      if (!compute_mapping<true>()) {
        return false;
      }
    }

    return true;
  }

  bool execute_area_mapping() {
    /* compute mapping using global area flow */
    while (iteration < ps.area_flow_rounds + 1) {
      compute_required_time();
      if (!compute_mapping<true>()) {
        return false;
      }
    }

    /* compute mapping using exact area */
    while (iteration < ps.ela_rounds + ps.area_flow_rounds + 1) {
      compute_required_time();
      if (!compute_mapping_exact<false>()) {
        return false;
      }
    }

    return true;
  }

#pragma endregion

#pragma region initiation 
public:
  double initialize() {
    std::cout<<"initialize\n";
    top_order.reserve( ntk.size() );
    topo_view<Ntk>( ntk ).foreach_node( [this]( auto n ) {
      top_order.push_back( n );
    } );
    
    /* match cuts with gates */
    compute_matches();

    compute_statistic();
    
    /* init the data structure */
    init_nodes();

    if (ps.baseline_v != "")
    {
      std::cout<<"size of np : "<<np.size()<<" ntk size is : "<<ntk.size()<<"\n";
      // auto [delay, area] = phyLS::_stime(lib_file, ps.baseline_v, ps.baseline_def);
      auto [delay, area] = phyLS::stime(lib_file, ps.baseline_v);
      std::cout<<"Baseline : Delay reward = "<<delay<<"\tArea reward = "<<area<<"\n";
    }

    return delay;
  }

  void init_nodes() {
    ntk.foreach_node([this](auto const& n, auto) {
      const auto index = ntk.node_to_index(n);
      auto& node_data = node_match[index];

      node_data.est_refs[0] = node_data.est_refs[1] = node_data.est_refs[2] =
          static_cast<float>(ntk.fanout_size(n));

      if (ntk.is_constant(n)) {
        /* all terminals have flow 1.0 */
        node_data.flows[0] = node_data.flows[1] = node_data.flows[2] = 0.0f;
        node_data.arrival[0] = node_data.arrival[1] = 0.0f;
        match_constants(index);
      } else if (ntk.is_ci(n)) {
        /* all terminals have flow 1.0 */
        node_data.flows[0] = node_data.flows[1] = node_data.flows[2] = 0.0f;
        node_data.arrival[0] = 0.0f;
        /* PIs have the negative phase implemented with an inverter */
        node_data.arrival[1] = lib_inv_delay;
      }
    });
  }

  template <typename CutType>
  void search_pins(uint32_t n, CutType* cut, uint16_t level = 0) {
    if (cut->size() <= 1) return;
    if (level > 5 || ntk.is_ci(ntk.index_to_node(n))) return;
    uint16_t level_ = level + 1;

    if (cut->pins.size() == 0) cut->pins.push_back(n);

    ntk.foreach_fanin(ntk.index_to_node(n), [&](signal<Ntk> const& fi) {
      uint32_t child = ntk.node_to_index(ntk.get_node(fi));
      bool in_leaves = false;
      for (uint32_t& i : *cut) {
        if (child == i) {
          in_leaves = true;
          break;
        }
      }
      if (!in_leaves) {
        if (level > 4) {
          cut->pins.push_back(child);
          return;
        }
        search_pins(child, cut, level_);
      } else {
        if (std::find(cut->pins.begin(), cut->pins.end(), n) == cut->pins.end())
          cut->pins.push_back(n);
        return;
      }
    });
  }

  void compute_matches() {
    /* match gates */
    ntk.foreach_gate([&](auto const& n) {
      const auto index = ntk.node_to_index(n);

      std::vector<cut_match_tech<NInputs>> node_matches;

      auto i = 0u;
      for (auto& cut : cuts.cuts(index)) {
        /* ignore unit cut */
        if (cut->size() == 1 && *cut->begin() == index) {
          (*cut)->data.ignore = true;
          continue;
        }
        if (cut->size() == 0) {
          (*cut)->data.ignore = true;
          continue;
        }
        if (cut->size() > NInputs) {
          /* Ignore cuts too big to be mapped using the library */
          (*cut)->data.ignore = true;
          continue;
        }
        const auto tt = cuts.truth_table(*cut);
        const auto fe = kitty::extend_to<6>(tt);
        auto fe_canon = fe;

        uint8_t negations_pos = 0;
        uint8_t negations_neg = 0;

        /* match positive polarity */
        if constexpr (Configuration == classification_type::p_configurations) {
          auto canon = kitty::exact_n_canonization(fe);
          fe_canon = std::get<0>(canon);
          negations_pos = std::get<1>(canon);
        }
        auto const supergates_pos = library.get_supergates(fe_canon);

        /* match negative polarity */
        if constexpr (Configuration == classification_type::p_configurations) {
          auto canon = kitty::exact_n_canonization(~fe);
          fe_canon = std::get<0>(canon);
          negations_neg = std::get<1>(canon);
        } else {
          fe_canon = ~fe;
        }
        auto const supergates_neg = library.get_supergates(fe_canon);

        if (supergates_pos != nullptr || supergates_neg != nullptr) {
          cut_match_tech<NInputs> match{{supergates_pos, supergates_neg},
                                        {negations_pos, negations_neg}};

          node_matches.push_back(match);
          (*cut)->data.match_index = i++;
        } else {
          /* Ignore not matched cuts */
          (*cut)->data.ignore = true;
        }
        search_pins(index, cut);
      }

      matches[index] = node_matches;
    });
  }

#pragma endregion

#pragma region statistic compute
  // compute matches for unmatched node (provide only AND, NAND, INV gates)
  void compute_unmapped_matches() {
    /* match gates */
    ntk.foreach_gate([&](auto const& n) {
      const auto index = ntk.node_to_index(n);

      std::vector<cut_match_tech<NInputs>> node_matches;

      auto i = 0u;
      for (auto& cut : cuts_unmapped.cuts(index)) {
        /* ignore unit cut */
        if (cut->size() == 1 && *cut->begin() == index) {
          (*cut)->data.ignore = true;
          continue;
        }
        if (cut->size() == 0) {
          (*cut)->data.ignore = true;
          continue;
        }
        if (cut->size() > NInputs) {
          /* Ignore cuts too big to be mapped using the library */
          (*cut)->data.ignore = true;
          continue;
        }
        const auto tt = cuts_unmapped.truth_table(*cut);
        const auto fe = kitty::extend_to<6>(tt);
        auto fe_canon = fe;

        uint8_t negations_pos = 0;
        uint8_t negations_neg = 0;

        /* match positive polarity */
        if constexpr (Configuration == classification_type::p_configurations) {
          auto canon = kitty::exact_n_canonization(fe);
          fe_canon = std::get<0>(canon);
          negations_pos = std::get<1>(canon);
        }
        auto const supergates_pos = cri_lib.get_supergates(fe_canon);

        /* match negative polarity */
        if constexpr (Configuration == classification_type::p_configurations) {
          auto canon = kitty::exact_n_canonization(~fe);
          fe_canon = std::get<0>(canon);
          negations_neg = std::get<1>(canon);
        } else {
          fe_canon = ~fe;
        }
        auto const supergates_neg = cri_lib.get_supergates(fe_canon);

        if (supergates_pos != nullptr || supergates_neg != nullptr) {
          cut_match_tech<NInputs> match{{supergates_pos, supergates_neg},
                                        {negations_pos, negations_neg}};

          node_matches.push_back(match);
          (*cut)->data.match_index = i++;
        } else {
          /* Ignore not matched cuts */
          (*cut)->data.ignore = true;
        }
        search_pins(index, cut);
      }

      unmapped_matches[index] = node_matches;
    });
  }

  // clear all elements in all queue
  void clear_queue()
  {
    area_queue.clear();
    delay_queue.clear();
    wirelength_queue.clear();
    totalwirelength_queue.clear();
  }

  // compute potential delay, area, wirelength of supergates
  void compute_statistic() 
  {
    std::cout<<"compute_statistic\n";
    // clear all elements in queue;
    clear_queue();

    for (uint8_t phase = 0; phase < 2; phase++) {
      ntk.foreach_gate([&](auto const& n) {
        const auto index = ntk.node_to_index(n);

        uint32_t cut_index = 0;
        for (auto& cut : cuts.cuts(index)) {
          /* trivial cuts or not matched cuts */
          if ((*cut)->data.ignore) {
            ++cut_index;
            continue;
          }
          auto const& supergates =
              matches[index][(*cut)->data.match_index].supergates;

          if (supergates[phase] == nullptr) {
            std::cerr<<"Supergate in matches is nullptr\n";
            ++cut_index;
            continue;
          }
          
          auto gate_position = compute_gate_position(*cut);
          // compute wirelength gain
          double wirelength = wirelength_gain(index, *cut, gate_position);
          // compute total wirelength gain
          double totalwirelength =
              totalwirelength_gain(index, *cut, gate_position);

          uint32_t supergate_index = 0;    
          for (auto const& gate : *supergates[phase]) {
            // std::cout << "current node = " << index;
            // std::cout << ": cut = { ";
            // for (auto l : *cut) std::cout << l << ", ";
            // std::cout << "}, ";
            // std::cout << "pins = { ";
            // for (auto i : (*cut).pins) std::cout << i << ", ";
            // std::cout << "}, ";

            // record corresponding index and cut_index of the supergate
            index_cut_supergate ics;
            ics.index = index;
            ics.phase = phase;
            ics.cut_index = cut_index;
            ics.supergate_index = supergate_index;
            supergate_index++;

            // compute area gain
            ics.area = area_gain(index, *cut, gate);
            // compute delay gain
            ics.delay = delay_gain(index, *cut, gate);
            ics.wirelength = wirelength;
            ics.totalwirelength = totalwirelength;
            
            // insert available match into queues
            area_queue.insert(ics);
            delay_queue.insert(ics);
            wirelength_queue.insert(ics);
            totalwirelength_queue.insert(ics);
            // std::cout << "delay = " << ics.delay << ", area = " << ics.area
            //           << ", wirelength = " << ics.wirelength
            //           << ", totalwirelength = " << ics.totalwirelength
            //           << std::endl;
          }
          ++cut_index;
        }
        return;
      });
    }
    if((area_queue.size() != delay_queue.size()) || (wirelength_queue.size() != area_queue.size()))
    {
      std::cerr << "queue size differs\n";
    }
    queue_size = area_queue.size();

    std::cout << "area_queue size = "<<area_queue.size()<<"\n";
    std::cout << "delay_queue size = "<<delay_queue.size()<<"\n";
    std::cout << "wirelength_queue = "<<wirelength_queue.size()<<"\n";
    std::cout << "totalwirelength_queue size = "<<totalwirelength_queue.size()<<"\n";
    // for (auto x : area_queue)
    //   std::cout << "index(" << x.index << ")-area(" << x.area << "), ";
    // std::cout << std::endl << std::endl;

    
    // for (auto x : delay_queue)
    //   std::cout << "index(" << x.index << ")-delay(" << x.delay << "), ";
    // std::cout << std::endl << std::endl;

    // for (auto x : wirelength_queue)
    //   std::cout << "index(" << x.index << ")-wirelength(" << x.wirelength
    //             << "), ";
    // std::cout << std::endl << std::endl;

    // for (auto x : totalwirelength_queue)
    //   std::cout << "index(" << x.index << ")-totalwirelength("
    //             << x.totalwirelength << "), ";
    // std::cout << std::endl << std::endl;
  }

  void compute_statistic_rough() 
  {
    std::cout<<"compute_statistic\n";
    // clear all elements in queue;
    clear_queue();

    for (uint8_t phase = 0; phase < 2; phase++) {
      ntk.foreach_gate([&](auto const& n) {
        const auto index = ntk.node_to_index(n);

        uint32_t cut_index = 0;
        index_cut_supergate delay_best_ics;
        index_cut_supergate area_best_ics;
        index_cut_supergate wirelength_best_ics;
        index_cut_supergate totalwirelength_best_ics;
        delay_best_ics.delay = std::numeric_limits<double>::lowest();
        area_best_ics.delay = std::numeric_limits<double>::lowest();
        wirelength_best_ics.delay = std::numeric_limits<double>::lowest();
        totalwirelength_best_ics.delay = std::numeric_limits<double>::lowest();

        for (auto& cut : cuts.cuts(index)) {
          /* trivial cuts or not matched cuts */
          if ((*cut)->data.ignore) {
            ++cut_index;
            continue;
          }
          auto const& supergates =
              matches[index][(*cut)->data.match_index].supergates;

          if (supergates[phase] == nullptr) {
            std::cerr<<"Supergate in matches is nullptr\n";
            ++cut_index;
            continue;
          }
          uint32_t supergate_index = 0;
          auto gate_position = compute_gate_position(*cut);
          // compute wirelength gain
          double wirelength = wirelength_gain(index, *cut, gate_position);
          // compute total wirelength gain
          double totalwirelength =
              totalwirelength_gain(index, *cut, gate_position);
          for (auto const& gate : *supergates[phase]) {
            std::cout << "current node = " << index;
            std::cout << ": cut = { ";
            for (auto l : *cut) std::cout << l << ", ";
            std::cout << "}, ";
            std::cout << "pins = { ";
            for (auto i : (*cut).pins) std::cout << i << ", ";
            std::cout << "}, ";

            // record corresponding index and cut_index of the supergate
            index_cut_supergate ics;
            ics.index = index;
            ics.phase = phase;
            ics.cut_index = cut_index;
            ics.supergate_index = supergate_index;
            supergate_index++;

            // compute statistic
            ics.area = area_gain(index, *cut, gate);
            ics.delay = delay_gain(index, *cut, gate);
            ics.wirelength = wirelength;
            ics.totalwirelength = totalwirelength;
            if (ics.delay > delay_best_ics.delay) delay_best_ics = ics;
            if (ics.area > area_best_ics.area) area_best_ics = ics;
            if (ics.wirelength > wirelength_best_ics.wirelength) wirelength_best_ics = ics;
            if (ics.totalwirelength > totalwirelength_best_ics.wirelength) 
              totalwirelength_best_ics = ics;
            
            // insert available match into queues
            std::cout << "delay = " << ics.delay << ", area = " << ics.area
                      << ", wirelength = " << ics.wirelength
                      << ", totalwirelength = " << ics.totalwirelength
                      << std::endl;
          }
          ++cut_index;
        }
        area_queue.insert(delay_best_ics);
        delay_queue.insert(area_best_ics);
        wirelength_queue.insert(wirelength_best_ics);
        totalwirelength_queue.insert(totalwirelength_best_ics);

        return;
      });
    }
    if((area_queue.size() != delay_queue.size()) || (wirelength_queue.size() != area_queue.size()))
    {
      std::cerr << "queue size differs\n";
    }
    queue_size = area_queue.size();

    std::cout << "area_queue size = "<<area_queue.size()<<"\n";
    std::cout << "delay_queue size = "<<delay_queue.size()<<"\n";
    std::cout << "wirelength_queue = "<<wirelength_queue.size()<<"\n";
    std::cout << "totalwirelength_queue size = "<<totalwirelength_queue.size()<<"\n";
  }

  double area_gain(uint32_t const& index, cut_t const& cut,
                   supergate<NInputs> const& gate) {
    // std::cout<<"wirelength gain\n";
    double local_area = gate.area;
    int cut_size = cut.pins.size();
    return -local_area / cut_size;
  }

  double delay_gain(uint32_t const& index, cut_t const& cut,
                    supergate<NInputs> const& gate) {
    // std::cout<<"wirelength gain\n";
    double local_delay = 0;
    auto ctr = 0u;
    for (auto l : cut) {
      if (local_delay < gate.tdelay[ctr]) local_delay = gate.tdelay[ctr];
      ++ctr;
    }
    int level = count_level(index, cut);
    return (-local_delay / level);
  }

  double wirelength_gain(uint32_t const& index, cut_t const& cut,
                         node_position const& gate_position) {
    // std::cout<<"wirelength gain\n";
    double local_wirelength = 0;
    for (auto l : cut) {
      double leaf_wirelength =
          std::abs(gate_position.x_coordinate - np[l].x_coordinate) +
          std::abs(gate_position.y_coordinate - np[l].y_coordinate);
      local_wirelength = std::max(leaf_wirelength, local_wirelength);
    }
    int level = count_level(index, cut);
    return -local_wirelength / level;
  }

  double totalwirelength_gain(uint32_t const& index, cut_t const& cut,
                              node_position const& gate_position) {
    // std::cout<<"wirelength gain\n";
    double local_totalwirelength = 0;
    for (auto l : cut) {
      double leaf_wirelength =
          std::abs(gate_position.x_coordinate - np[l].x_coordinate) +
          std::abs(gate_position.y_coordinate - np[l].y_coordinate);
      local_totalwirelength += leaf_wirelength;
    }
    int cut_size = cut.pins.size();
    return -local_totalwirelength / cut_size;
  }

  int count_level(uint32_t const& index, cut_t const& cut, uint32_t level = 0) 
  {
    if ( level > 4 ) return level;
    if ( level > 10 ) throw std::out_of_range("couting level has exceeded 10");
    int lev = level + 1;
    ntk.foreach_fanin(ntk.index_to_node(index), [&](auto const& fi) {
      auto findex = ntk.node_to_index(ntk.get_node(fi));
      if (std::find(cut.begin(), cut.end(), findex) == cut.end()) {
        int l = count_level(findex, cut, lev);
        if (l > level) {
          level = lev;
        }
      }
    });
    
    return lev;
  }

#pragma endregion

#pragma region forward choice
enum Action { delta_delay, delta_area, delta_wirlength };

public:
  // external interface
  bool terminal()
  {
    return _terminal;
  }

private:
  bool terminated()
  {
    if ( _terminal ) return _terminal;

    _terminal = true;
    int unset = 0;
    set_nodes = 0;
    ntk.foreach_gate([&](auto const& n){
     auto index = ntk.node_to_index(n);
     auto const& node_data = node_match[index];
     if ( !node_data.set_flag[0] && !node_data.set_flag[1] )
     {
      _terminal = false;
      unset++;
     }
     else set_nodes++;
    });
    std::cout<<"Set nodes = "<<set_nodes<<" Unset nodes = "<<unset<<"\n"; 
    return _terminal;
  }

public:
  void reinit()
  {
    std::cout<<"reinit\n";
    node_match.assign(ntk.size(), node_match_tech<NInputs>());
    bool inited = true;
    for (auto const& node_data : node_match)
    {
      if (node_data.best_supergate[0] != nullptr && node_data.best_supergate[0] != nullptr)
      {
        inited = false;
      }
    }
    if (inited) std::cout<<"uninitialized\n";
    set_nodes = 0;
    _reward = 0;
    _terminal = false;
    compute_statistic();
    init_nodes();
  }

  double take_action(int choice, int const& depth = 0)
  {
    if ( choice < 0 ) 
    {
      std::cerr << "choice is less than 0\n";
      throw std::out_of_range("Invalid action choice");
      exit(0);
    }
    // else if ( choice < 1) {return forward(&delay_queue);}
    // else if ( choice < 2) {return forward(&delay_queue);}
    // else if ( choice < 3) {return forward(&delay_queue);}
    // else if ( choice < 4) {return forward(&delay_queue);}
    else if ( choice < 1) {return forward(&delay_queue, depth);}
    else if ( choice < 2) {return forward(&area_queue, depth);}
    else if ( choice < 3) {return forward(&wirelength_queue, depth);}
    else if ( choice < 4) {return forward(&totalwirelength_queue, depth);}
    else 
    {
      std::cerr << "choice is more than 3\n";
      throw std::out_of_range("Invalid action choice");
      exit(0);
    }
  }

  template<typename Comparator>
  double forward(std::multiset<index_cut_supergate, Comparator>* queue, int const& depth = 0)
  {
    if (queue == nullptr) {
      std::cerr << "queue is null\n";
      exit(0);
    }


    bool suc = !compute_mapping<false>();
    if ( suc )
    {
      std::cerr<<"delay mapping flow failed\n";
    }
   
    // compute_required_time();
    // if ( !compute_mapping<true>() )
    // {
    //   std::cerr<<"global area mapping flow failed\n";
    // }

    // in top_x, there are top x% elements of chosen queue
    std::vector<index_cut_supergate> top_x;
    get_top_percent(*queue, 0.1, top_x);

    std::cout<<"taken "<<top_x.size()<<" supergates to conduct match\n";

    // implement selected matches to node
    for (index_cut_supergate const& ics : top_x) {
      auto& node_data = node_match[ics.index];
      // if the target node has set, skip all other supergates of the node
      if (node_data.set_flag[ics.phase])
        continue;
      else
      {
        node_data.set_flag[ics.phase] = true;
      }
      auto const& cut = cuts.cuts(ics.index)[ics.cut_index];
      auto const& supergates = matches[ics.index][cut->data.match_index].supergates[ics.phase];
      auto const& gate = (*supergates)[ics.supergate_index];
      auto const negation = matches[ics.index][cut->data.match_index].negations[ics.phase];
      node_data.best_supergate[ics.phase] = &gate;
      node_data.phase[ics.phase] = gate.polarity ^ negation;
      node_data.best_cut[ics.phase] = ics.cut_index;
    }



    terminated();

    // finalize the netlist with lib with only AND, NAND, INV gates
    compute_rest_mapping<false>();

    if(set_nodes > (0.6 * ntk.size()))
    {
      if (repair())
      {
        std::cout<<"netlist repaired\n";
      }
    }   

    // get timing evaluation by STA;
    auto [res, old2new] = initialize_map_network();
    insert_buffers();
    finalize_cover<map_ntk_t>(res, old2new);
    auto [delay, area] = compute_reward(res);
    _reward = delay;
    _mulReward = delay * area;

    std::cout << "area_queue size = "<<area_queue.size()<<"\n";
    std::cout << "delay_queue size = "<<delay_queue.size()<<"\n";
    std::cout << "wirelength_queue = "<<wirelength_queue.size()<<"\n";
    std::cout << "totalwirelength_queue size = "<<totalwirelength_queue.size()<<"\n";

    eventual_res = res;
    
    if (ps.strategy == map_params::amd) return _mulReward;
    return _reward;
  }

  /* repair the mapped netlist */
  bool repair()
  {
    compute_required_time();
    if (!compute_mapping_exact<false>()) return false;
    return true;
  }

  // extract top x elements from a queue (actually it is a "set" type)
  template <typename SetType>
  void get_top_percent(SetType& queue, double const& per,
                       std::vector<typename SetType::value_type>& topElements) {
    std::cout<<"queue size = "<<queue_size<<"\t";
    size_t topCount = static_cast<size_t>(queue_size * per);  // number of elements should be taken
    if (topCount > queue.size() ) {
      topCount = queue.size();
    }
    std::cout<<"top count = "<<topCount<<"\n";

    topElements.reserve(topCount);

    // copy top elements
    auto it = std::next(
        queue.begin(),
        static_cast<typename SetType::difference_type>(queue.size() - topCount));
    std::copy(it, queue.end(), std::back_inserter(topElements));

    // these code should be revised to be more flexible
    queue.erase(it, queue.end());
  }

  template <bool DO_AREA>
  bool compute_rest_mapping() {
    for (auto const& n : top_order) {
      if (ntk.is_constant(n) || ntk.is_ci(n)) {
        continue;
      }

      /* match positive phase */
      match_unmapped_phase<DO_AREA>(n, 0u);

      /* match negative phase */
      match_unmapped_phase<DO_AREA>(n, 1u);

      /* try to drop one phase */
      match_drop_phase<DO_AREA, false>(n, 0);
    }

    double area_old = area;
    bool success = set_mapping_refs<false>();

    /* round stats */
    if (ps.verbose) {
      std::stringstream stats{};
      float area_gain = 0.0f;

      if (iteration != 1) area_gain = float((area_old - area) / area_old * 100);

      if constexpr (DO_AREA) {
        stats << fmt::format(
            "[i] AreaFlow : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n",
            delay, area, area_gain);
      } else {
        stats << fmt::format(
            "[i] Delay    : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n",
            delay, area, area_gain);
      }
      st.round_stats.push_back(stats.str());
      std::cout<<stats.str();
    }

    return success;
  }

  template <bool DO_AREA, bool MCTS = true>
  void match_unmapped_phase(node<Ntk> const& n, uint8_t phase) {
    auto index = ntk.node_to_index(n);
    auto& node_data = node_match[index];

    double best_arrival = std::numeric_limits<double>::max();
    double best_area_flow = std::numeric_limits<double>::max();
    float best_area = std::numeric_limits<float>::max();
    uint32_t best_size = UINT32_MAX;
    uint8_t best_cut = 0u;
    uint8_t best_phase = 0u;
    uint8_t cut_index = 0u;
    auto& cut_matches = matches[index];
    supergate<NInputs> const* best_supergate = node_data.best_supergate[phase];
    if (best_supergate != nullptr) {
      auto const& cut = cuts.cuts(index)[node_data.best_cut[phase]];

      best_phase = node_data.phase[phase];
      best_arrival = 0.0f;
      // best_area_flow = best_supergate->area + cut_leaves_flow(cut, n, phase);
      best_area = best_supergate->area;
      best_cut = node_data.best_cut[phase];
      best_size = cut.size();

      auto ctr = 0u;
      for (auto l : cut) {
        double arrival_pin = node_match[l].arrival[(best_phase >> ctr) & 1] +
                             best_supergate->tdelay[ctr];
        best_arrival = std::max(best_arrival, arrival_pin);
        ++ctr;
      }
    }
    /* foreach cut */
    // for (auto& cut : cuts.cuts(index)) {
    //   /* trivial cuts or not matched cuts */
    //   if ((*cut)->data.ignore) {
    //     ++cut_index;
    //     continue;
    //   }

      // auto const& supergates = cut_matches[(*cut)->data.match_index].supergates;
      // auto const negation =
      //     cut_matches[(*cut)->data.match_index].negations[phase];

      // if (supergates[phase] == nullptr) {
      //   ++cut_index;
      //   continue;
      // }

      // for (auto const& gate : *supergates[phase]) {
      //   uint8_t gate_polarity = gate.polarity ^ negation;
      //   node_data.phase[phase] = gate_polarity;
      //   double area_local = gate.area + cut_leaves_flow(*cut, n, phase);
      //   double worst_arrival = 0.0f;

      //   auto ctr = 0u;
      //   for (auto l : *cut) {
      //     double arrival_pin =
      //         node_match[l].arrival[(gate_polarity >> ctr) & 1] +
      //         gate.tdelay[ctr];
      //     worst_arrival = std::max(worst_arrival, arrival_pin);
      //     ++ctr;
      //   }

      //   if constexpr (DO_AREA) {
      //     if (worst_arrival > node_data.required[phase] + epsilon) continue;
      //   }

        // std::cout << "compare map\n";
        // if (compare_map<DO_AREA>(worst_arrival, best_arrival, area_local,
        //                          best_area_flow, cut->size(), best_size)) {
        //   best_arrival = worst_arrival;
        //   best_area_flow = area_local;
        //   best_size = cut->size();
        //   best_cut = cut_index;
        //   best_area = gate.area;
        //   best_phase = gate_polarity;
        //   best_supergate = &gate;
        // }
      // }
      // ++cut_index;
    // }
    node_data.flows[phase] = best_area_flow;
    node_data.arrival[phase] = best_arrival;
    node_data.area[phase] = best_area;
    node_data.best_cut[phase] = best_cut;
    node_data.phase[phase] = best_phase;
    node_data.best_supergate[phase] = best_supergate;
  }

  std::vector<node_position> get_mappedNtk_position(map_ntk_t res)
  {
    std::cout<<"size of binding view is "<<res.size()<<"\n";
    std::vector<node_position> netlist_position;
    // netlist_position.push_back({0,0});
    node_map<std::vector<uint32_t>, Ntk, std::unordered_map<typename Ntk::node, std::vector<uint32_t>>> po_nodes( ntk );
    res.foreach_po([&](auto const& f, auto const& i){
      po_nodes[f].push_back( i );
    });
    int cpo = 0;
    int cpi = 0;
    int cnode = 0;
    int cconstant = 0;
    res.foreach_node([&](auto const& n){
      index_phase_pair const& ipp = res2ntk[n];
      // std::cout<<"for node "<<n<<" ";

      /* insert position of constant */
      if ( res.is_constant( n ) )
      {
        // std::cout<<"gate is constant \n";
        cconstant++;
        return;
      }
      /* insert position of PI */
      else if ( ntk.is_pi( ipp.node_index ) )
      {
        cpi++;
        // std::cout<<"gate is PI ";
        if ( res.has_binding( n ) )
        {
          if ( res.get_binding_index(n) == lib_inv_id )
          {
            // std::cout<<"detected a inverter at ";
            netlist_position.emplace_back( 
            node_position(
              (0.5 * np[res2ntk[n].pins[1]].x_coordinate + 0.5 * np[res2ntk[n].pins[0]].x_coordinate),
              (0.5 * np[res2ntk[n].pins[1]].y_coordinate + 0.5 * np[res2ntk[n].pins[0]].y_coordinate)
            )
            ); 
          return;
          }
        }      
        netlist_position.emplace_back(np[ipp.node_index]);
      }
      /* insert position of std cell */
      else if ( res.has_binding( n ) ) { 
        cnode++;
        if ( res.get_binding_index(n) == lib_inv_id )
        {
          // std::cout<<"detected a inverter at ";
          auto& inv_node = netlist_position.emplace_back(
            node_position(
              (0.5 * np[res2ntk[n].pins[1]].x_coordinate + 0.5 * np[res2ntk[n].pins[0]].x_coordinate),
              (0.5 * np[res2ntk[n].pins[1]].y_coordinate + 0.5 * np[res2ntk[n].pins[0]].y_coordinate)
            )
          );
          // std::cout<<"{ "<<inv_node.x_coordinate<<", "<<inv_node.y_coordinate<<" }\n";
          return;
        }
        // else{
          auto const& node_data = node_match[ipp.node_index];
          auto const& cut = cuts.cuts(ipp.node_index)[node_data.best_cut[ipp.node_phase]];
          netlist_position.emplace_back(compute_gate_position(cut));
        // }    
      }
    });
    /* collect all PO nodes */
    res.foreach_po([&](auto const& o ){
      // std::cout<<"for node "<<o<<" ";
      // std::cout<<"gate is PO \n";
      cpo++;
      int po = ntk.size() + po_nodes[o][0];
      netlist_position.emplace_back(np[po]);
    });
    std::cout<<"in this iteration, pi : "<<cpi<<", po : "<<cpo<<", cnode : "<<cnode<<", cconstant : "<<cconstant<<"\n";

    return netlist_position;
  }

  std::pair<double, double> compute_reward(map_ntk_t const& res) 
  {
    std::string filename = ps.result_dir + "/" + "temp.v";
    write_verilog_with_binding(res, filename);
    auto netlist_position = get_mappedNtk_position(res);
    // double reward_delay, reward_area;
    // phyLS::stime_of_res(lib_file, filename, netlist_position, reward_delay, reward_area);
    auto [reward_delay, reward_area] = phyLS::stime(lib_file, filename);
    std::stringstream state;
    state << fmt::format("Delay reward = {:>12.2f}  Area reward = {:>12.2f}\n", reward_delay, reward_area);
    std::cout<<state.str();
    return std::make_pair(reward_delay, reward_area);
  }

#pragma endregion

#pragma region savingResult region
  void record_bestResult()
  {
    if (ps.strategy == map_params::d_only){
      auto const& file_path = ps.best_result_file;
      std::cout<<"recording best reward result at "<<file_path<<"\n";
      write_verilog_with_binding(eventual_res, file_path);
    }   
  }

  void record_result(std::string filename)
  {
    std::cout<<"recording "<<filename<<"\n";
    auto file_path = ps.result_dir + "/" + filename + ".v";
    write_verilog_with_binding(eventual_res, file_path);
  }

  void record_mulResult()
  {
    if (ps.strategy == map_params::amd){
      auto const& file_path = ps.best_mulResult_file;
      std::cout<<"recording best reward result at "<<file_path<<"\n";
      write_verilog_with_binding(eventual_res, file_path);
    }
  }
#pragma endregion

  template <bool DO_AREA>
  bool compute_mapping() {
    for (auto const& n : top_order) {
      if (ntk.is_constant(n) || ntk.is_ci(n)) {
        continue;
      }

      /* match positive phase */
      // std::cout << "match phase1\n";
      match_phase<DO_AREA>(n, 0u);

      /* match negative phase */
      // std::cout << "match phase2\n";
      match_phase<DO_AREA>(n, 1u);

      /* try to drop one phase */
      // std::cout << "match drop phase1\n";
      match_drop_phase<DO_AREA, false>(n, 0);
    }

    double area_old = area;
    bool success = set_mapping_refs<false>();

    /* round stats */
    if (ps.verbose) {
      std::stringstream stats{};
      float area_gain = 0.0f;

      if (iteration != 1) area_gain = float((area_old - area) / area_old * 100);

      if constexpr (DO_AREA) {
        stats << fmt::format(
            "[i] AreaFlow : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n",
            delay, area, area_gain);
      } else {
        stats << fmt::format(
            "[i] Delay    : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n",
            delay, area, area_gain);
      }
      st.round_stats.push_back(stats.str());
    }

    return success;
  }

  template <bool SwitchActivity>
  bool compute_mapping_exact() {
    for (auto const& n : top_order) {
      if (ntk.is_constant(n) || ntk.is_ci(n)) continue;

      auto index = ntk.node_to_index(n);
      auto& node_data = node_match[index];

      /* recursively deselect the best cut shared between
       * the two phases if in use in the cover */
      if (node_data.same_match && node_data.map_refs[2] != 0) {
        if (node_data.best_supergate[0] != nullptr)
          cut_deref<SwitchActivity>(cuts.cuts(index)[node_data.best_cut[0]], n,
                                    0u);
        else
          cut_deref<SwitchActivity>(cuts.cuts(index)[node_data.best_cut[1]], n,
                                    1u);
      }

      /* match positive phase */
      match_phase_exact<SwitchActivity>(n, 0u);

      /* match negative phase */
      match_phase_exact<SwitchActivity>(n, 1u);

      /* try to drop one phase */
      match_drop_phase<true, true>(n, 0);
    }

    double area_old = area;
    bool success = set_mapping_refs<true>();

    /* round stats */
    if (ps.verbose) {
      float area_gain = float((area_old - area) / area_old * 100);
      std::stringstream stats{};
      if constexpr (SwitchActivity)
        stats << fmt::format(
            "[i] Switching: Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n",
            delay, area, area_gain);
      else
        stats << fmt::format(
            "[i] Area     : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n",
            delay, area, area_gain);
      st.round_stats.push_back(stats.str());
    }

    return success;
  }

  template <bool DO_WIRE, bool DO_DELAY>
  bool compute_mapping_wirelength() {
    std::cout << "size of match position is " << match_position.size()
              << ". np size is " << np.size() << "\n";
    for (auto const& n : top_order) {
      std::cout << "copy np to match position\n";
      if (ntk.is_constant(n) || ntk.is_ci(n)) {
        uint32_t idx = ntk.node_to_index(n);
        match_position[idx] = np[idx];
        continue;
      }

      std::cout << "node match is as big as " << node_match.size() << "\n";
      std::cout << "input size is " << ntk._storage->inputs.size() << "\n";
      std::cout << "node size is " << ntk._storage->nodes.size() << "\n";
      std::cout << "output size is " << ntk._storage->outputs.size() << "\n";
      /* match positive wire&delay phase */
      std::cout << "matching wirelength\n";
      match_wirelength<DO_WIRE, DO_DELAY>(n, 0u);

      /* match negative wire&delay phase */
      match_wirelength<DO_WIRE, DO_DELAY>(n, 1u);

      /* try to drop one delay phase */
      std::cout << " match drop phase\n";
      match_drop_phase<DO_WIRE, false>(n, 0);
    }

    bool success = set_mapping_refs_wirelength<false>();

    /* round stats */
    if (ps.verbose) {
      std::stringstream stats{};
      stats << fmt::format(
          "[i] Wire     : Delay = {:>12.2f}  Area = {:>12.2f}  Wirelength = "
          "{:>12.2f}\n",
          delay, area, wirelength);
      st.round_stats.push_back(stats.str());
    }

    std::cout << "compute mapping wirelength done\n";
    return success;
  }

  template <bool DO_WIRE, bool DO_DELAY, bool DO_AREA>
  bool compute_mapping_wirelength_balance() {
    for (auto const& n : top_order) {
      if (ntk.is_constant(n) || ntk.is_ci(n)) {
        uint32_t idx = ntk.node_to_index(n);
        match_position[idx] = np[idx];
        continue;
      }
      /* match positive wire&delay phase */
      match_wirelength_balance<DO_WIRE, DO_DELAY, DO_AREA>(n, 0u);

      /* match negative wire&delay phase */
      match_wirelength_balance<DO_WIRE, DO_DELAY, DO_AREA>(n, 1u);

      /* try to drop one delay phase */
      match_drop_phase<DO_WIRE, false>(n, 0);
    }

    bool success = set_mapping_refs_wirelength<false>();

    /* round stats */
    if (ps.verbose) {
      std::stringstream stats{};
      stats << fmt::format(
          "[i] Wire     : Delay = {:>12.2f}  Area = {:>12.2f}  Wirelength = "
          "{:>12.2f}\n",
          delay, area, wirelength);
      st.round_stats.push_back(stats.str());
    }

    return success;
  }

  template <bool ELA>
  bool set_mapping_refs() {
    // std::cout << "set_mapping refs\n";
    const auto coef = 1.0f / (2.0f + (iteration + 1) * (iteration + 1));

    if constexpr (!ELA) {
      for (auto i = 0u; i < node_match.size(); ++i) {
        node_match[i].map_refs[0] = node_match[i].map_refs[1] =
            node_match[i].map_refs[2] = 0u;
      }
    }

    /* compute the current worst delay and update the mapping refs */
    wirelength = 0.0f;
    total_wirelength = 0.0f;
    delay = 0.0f;
    ntk.foreach_co([this](auto s) {
      const auto index = ntk.node_to_index(ntk.get_node(s));

      if (ntk.is_complemented(s)) {
        delay = std::max(delay, node_match[index].arrival[1]);
        wirelength = std::max(wirelength, node_match[index].wirelength[1]);
        total_wirelength += node_match[index].total_wirelength[1];
      } else {
        delay = std::max(delay, node_match[index].arrival[0]);
        wirelength = std::max(wirelength, node_match[index].wirelength[0]);
        total_wirelength += node_match[index].total_wirelength[0];
      }

      if constexpr (!ELA) {
        node_match[index].map_refs[2]++;
        if (ntk.is_complemented(s))
          node_match[index].map_refs[1]++;
        else
          node_match[index].map_refs[0]++;
      }
    });

    /* compute current area and update mapping refs in top-down order */
    area = 0.0f;
    for (auto it = top_order.rbegin(); it != top_order.rend(); ++it) {
      const auto index = ntk.node_to_index(*it);
      auto& node_data = node_match[index];

      /* skip constants and PIs */
      if (ntk.is_constant(*it)) {
        if (node_match[index].map_refs[2] > 0u) {
          /* if used and not available in the library launch a mapping error */
          if (node_data.best_supergate[0] == nullptr &&
              node_data.best_supergate[1] == nullptr) {
            std::cerr << "[i] MAP ERROR: technology library does not contain "
                         "constant gates, impossible to perform mapping"
                      << std::endl;
            st.mapping_error = true;
            return false;
          }
        }
        continue;
      } else if (ntk.is_ci(*it)) {
        if (node_match[index].map_refs[1] > 0u) {
          /* Add inverter area over the negated fanins */
          area += lib_inv_area;
        }
        continue;
      }

      /* continue if not referenced in the cover */
      if (node_match[index].map_refs[2] == 0u) continue;

      unsigned use_phase = node_data.best_supergate[0] == nullptr ? 1u : 0u;

      if (node_data.best_supergate[use_phase] == nullptr) {
        /* Library is not complete, mapping is not possible */
        std::cerr << "[i] MAP ERROR: technology library is not complete, "
                     "impossible to perform mapping"
                  << std::endl;
        st.mapping_error = true;
        return false;
      }

      if (node_data.same_match || node_data.map_refs[use_phase] > 0) {
        if constexpr (!ELA) {
          auto const& best_cut =
              cuts.cuts(index)[node_data.best_cut[use_phase]];
          auto ctr = 0u;

          for (auto const leaf : best_cut) {
            node_match[leaf].map_refs[2]++;
            if ((node_data.phase[use_phase] >> ctr++) & 1)
              node_match[leaf].map_refs[1]++;
            else
              node_match[leaf].map_refs[0]++;
          }
        }
        area += node_data.area[use_phase];
        if (node_data.same_match && node_data.map_refs[use_phase ^ 1] > 0) {
          area += lib_inv_area;
        }
      }

      /* invert the phase */
      use_phase = use_phase ^ 1;

      /* if both phases are implemented and used */
      if (!node_data.same_match && node_data.map_refs[use_phase] > 0) {
        if constexpr (!ELA) {
          auto const& best_cut =
              cuts.cuts(index)[node_data.best_cut[use_phase]];
          auto ctr = 0u;
          for (auto const leaf : best_cut) {
            node_match[leaf].map_refs[2]++;
            if ((node_data.phase[use_phase] >> ctr++) & 1)
              node_match[leaf].map_refs[1]++;
            else
              node_match[leaf].map_refs[0]++;
          }
        }
        area += node_data.area[use_phase];
      }
    }

    /* blend estimated references */
    for (auto i = 0u; i < ntk.size(); ++i) {
      node_match[i].est_refs[2] =
          coef * node_match[i].est_refs[2] +
          (1.0f - coef) *
              std::max(1.0f, static_cast<float>(node_match[i].map_refs[2]));
      node_match[i].est_refs[1] =
          coef * node_match[i].est_refs[1] +
          (1.0f - coef) *
              std::max(1.0f, static_cast<float>(node_match[i].map_refs[1]));
      node_match[i].est_refs[0] =
          coef * node_match[i].est_refs[0] +
          (1.0f - coef) *
              std::max(1.0f, static_cast<float>(node_match[i].map_refs[0]));
    }

    ++iteration;
    return true;
  }

  template <bool ELA>
  bool set_mapping_refs_wirelength() {
    const auto coef = 1.0f / (2.0f + (iteration + 1) * (iteration + 1));

    if constexpr (!ELA) {
      for (auto i = 0u; i < node_match.size(); ++i)
        node_match[i].map_refs[0] = node_match[i].map_refs[1] =
            node_match[i].map_refs[2] = 0u;
    }

    /* compute the current worst wirelength&delay and update the mapping refs */
    wirelength = 0.0f;
    total_wirelength = 0.0f;
    delay = 0.0f;
    ntk.foreach_co([this](auto s) {
      const auto index = ntk.node_to_index(ntk.get_node(s));

      if (ntk.is_complemented(s)) {
        delay = std::max(delay, node_match[index].arrival[1]);
        wirelength = std::max(wirelength, node_match[index].wirelength[1]);
        total_wirelength += node_match[index].total_wirelength[1];
      } else {
        delay = std::max(delay, node_match[index].arrival[0]);
        wirelength = std::max(wirelength, node_match[index].wirelength[0]);
        total_wirelength += node_match[index].total_wirelength[0];
      }

      if constexpr (!ELA) {
        node_match[index].map_refs[2]++;
        if (ntk.is_complemented(s))
          node_match[index].map_refs[1]++;
        else
          node_match[index].map_refs[0]++;
      }
    });

    /* compute current area and update mapping refs in top-down order */
    area = 0.0f;
    for (auto it = top_order.rbegin(); it != top_order.rend(); ++it) {
      const auto index = ntk.node_to_index(*it);
      auto& node_data = node_match[index];

      /* skip constants and PIs */
      if (ntk.is_constant(*it)) {
        if (node_match[index].map_refs[2] > 0u) {
          /* if used and not available in the library launch a mapping error */
          if (node_data.best_supergate[0] == nullptr &&
              node_data.best_supergate[1] == nullptr) {
            std::cerr << "[i] MAP ERROR: technology library does not contain "
                         "constant gates, impossible to perform mapping"
                      << std::endl;
            st.mapping_error = true;
            return false;
          }
        }
        continue;
      } else if (ntk.is_ci(*it)) {
        if (node_match[index].map_refs[1] > 0u) {
          /* Add inverter area over the negated fanins */
          area += lib_inv_area;
        }
        continue;
      }

      /* continue if not referenced in the cover */
      if (node_match[index].map_refs[2] == 0u) continue;

      unsigned use_phase = node_data.best_supergate[0] == nullptr ? 1u : 0u;

      if (node_data.best_supergate[use_phase] == nullptr) {
        /* Library is not complete, mapping is not possible */
        std::cerr << "[i] MAP ERROR: technology library is not complete, "
                     "impossible to perform mapping"
                  << std::endl;
        st.mapping_error = true;
        return false;
      }

      if (node_data.same_match || node_data.map_refs[use_phase] > 0) {
        if constexpr (!ELA) {
          auto const& best_cut =
              cuts.cuts(index)[node_data.best_cut[use_phase]];
          auto ctr = 0u;

          for (auto const leaf : best_cut) {
            node_match[leaf].map_refs[2]++;
            if ((node_data.phase[use_phase] >> ctr++) & 1)
              node_match[leaf].map_refs[1]++;
            else
              node_match[leaf].map_refs[0]++;
          }
        }
        area += node_data.area[use_phase];
        if (node_data.same_match && node_data.map_refs[use_phase ^ 1] > 0)
          area += lib_inv_area;
      }

      /* invert the phase */
      use_phase = use_phase ^ 1;

      /* if both phases are implemented and used */
      if (!node_data.same_match && node_data.map_refs[use_phase] > 0) {
        if constexpr (!ELA) {
          auto const& best_cut =
              cuts.cuts(index)[node_data.best_cut[use_phase]];
          auto ctr = 0u;
          for (auto const leaf : best_cut) {
            node_match[leaf].map_refs[2]++;
            if ((node_data.phase[use_phase] >> ctr++) & 1)
              node_match[leaf].map_refs[1]++;
            else
              node_match[leaf].map_refs[0]++;
          }
        }
        area += node_data.area[use_phase];
      }
    }

    /* blend estimated references */
    for (auto i = 0u; i < ntk.size(); ++i) {
      node_match[i].est_refs[2] =
          coef * node_match[i].est_refs[2] +
          (1.0f - coef) *
              std::max(1.0f, static_cast<float>(node_match[i].map_refs[2]));
      node_match[i].est_refs[1] =
          coef * node_match[i].est_refs[1] +
          (1.0f - coef) *
              std::max(1.0f, static_cast<float>(node_match[i].map_refs[1]));
      node_match[i].est_refs[0] =
          coef * node_match[i].est_refs[0] +
          (1.0f - coef) *
              std::max(1.0f, static_cast<float>(node_match[i].map_refs[0]));
    }

    ++iteration;
    return true;
  }

  void compute_required_time() {
    std::cout << "compute requie time\n";
    for (auto i = 0u; i < node_match.size(); ++i) {
      node_match[i].required[0] = node_match[i].required[1] =
          std::numeric_limits<double>::max();
    }

    /* return in case of `skip_delay_round` */
    if (iteration == 0) return;

    auto required = delay;

    if (ps.required_time != 0.0f) {
      /* Global target time constraint */
      if (ps.required_time < delay - epsilon) {
        if (!ps.skip_delay_round && iteration == 1)
          std::cerr << fmt::format(
                           "[i] MAP WARNING: cannot meet the target required "
                           "time of {:.2f}",
                           ps.required_time)
                    << std::endl;
      } else {
        required = ps.required_time;
      }
    }

    /* set the required time at POs */
    ntk.foreach_co([&](auto const& s) {
      const auto index = ntk.node_to_index(ntk.get_node(s));
      // std::cout << "index is " << index << "\t";
      if (ntk.is_complemented(s))
        node_match[index].required[1] = required;
      else
        node_match[index].required[0] = required;
    });

    /* propagate required time to the PIs */
    // std::cout << "compute requie time3\n";
    for (auto it = top_order.rbegin(); it != top_order.rend(); ++it) {
      if (ntk.is_ci(*it) || ntk.is_constant(*it)) break;

      const auto index = ntk.node_to_index(*it);

      if (node_match[index].map_refs[2] == 0) continue;

      auto& node_data = node_match[index];

      unsigned use_phase = node_data.best_supergate[0] == nullptr ? 1u : 0u;
      unsigned other_phase = use_phase ^ 1;

      assert(node_data.best_supergate[0] != nullptr ||
             node_data.best_supergate[1] != nullptr);
      assert(node_data.map_refs[0] || node_data.map_refs[1]);

      /* propagate required time over the output inverter if present */
      if (node_data.same_match && node_data.map_refs[other_phase] > 0) {
        node_data.required[use_phase] =
            std::min(node_data.required[use_phase],
                     node_data.required[other_phase] - lib_inv_delay);
      }

      if (node_data.same_match || node_data.map_refs[use_phase] > 0) {
        auto ctr = 0u;
        auto best_cut = cuts.cuts(index)[node_data.best_cut[use_phase]];
        auto const& supergate = node_data.best_supergate[use_phase];
        for (auto leaf : best_cut) {
          auto phase = (node_data.phase[use_phase] >> ctr) & 1;
          node_match[leaf].required[phase] =
              std::min(node_match[leaf].required[phase],
                       node_data.required[use_phase] - supergate->tdelay[ctr]);
          ++ctr;
        }
      }

      // std::cout << "compute requie time4\n";
      if (!node_data.same_match && node_data.map_refs[other_phase] > 0) {
        auto ctr = 0u;
        auto best_cut = cuts.cuts(index)[node_data.best_cut[other_phase]];
        auto const& supergate = node_data.best_supergate[other_phase];
        for (auto leaf : best_cut) {
          auto phase = (node_data.phase[other_phase] >> ctr) & 1;
          node_match[leaf].required[phase] = std::min(
              node_match[leaf].required[phase],
              node_data.required[other_phase] - supergate->tdelay[ctr]);
          ++ctr;
        }
      }
    }
    std::cout << "compute requie time done\n";
  }

  void compute_wirelength_required_time() {
    for (auto i = 0u; i < node_match.size(); ++i) {
      node_match[i].required_wirelength[0] =
          node_match[i].required_wirelength[1] =
              std::numeric_limits<double>::max();
    }

    /* return in case of `skip_delay_round` */
    if (iteration == 0) return;
    auto required = wirelength;

    /* set the required time at POs */
    ntk.foreach_co([&](auto const& s) {
      const auto index = ntk.node_to_index(ntk.get_node(s));
      if (ntk.is_complemented(s))
        node_match[index].required_wirelength[1] = required;
      else
        node_match[index].required_wirelength[0] = required;
    });

    /* propagate required time to the PIs */
    for (auto it = top_order.rbegin(); it != top_order.rend(); ++it) {
      if (ntk.is_ci(*it) || ntk.is_constant(*it)) break;

      const auto index = ntk.node_to_index(*it);

      if (node_match[index].map_refs[2] == 0) continue;

      auto& node_data = node_match[index];

      unsigned use_phase = node_data.best_supergate[0] == nullptr ? 1u : 0u;
      unsigned other_phase = use_phase ^ 1;

      assert(node_data.best_supergate[0] != nullptr ||
             node_data.best_supergate[1] != nullptr);
      assert(node_data.map_refs[0] || node_data.map_refs[1]);

      /* propagate required time over the output inverter if present */
      if (node_data.same_match && node_data.map_refs[other_phase] > 0) {
        node_data.required_wirelength[use_phase] =
            std::min(node_data.required_wirelength[use_phase],
                     node_data.required_wirelength[other_phase]);
      }

      if (node_data.same_match || node_data.map_refs[use_phase] > 0) {
        auto ctr = 0u;
        auto best_cut = cuts.cuts(index)[node_data.best_cut[use_phase]];
        // compute gate position of root
        node_position np = compute_gate_position(best_cut);
        auto const& supergate = node_data.best_supergate[use_phase];
        for (auto leaf : best_cut) {
          auto phase = (node_data.phase[use_phase] >> ctr) & 1;
          // compute gate position of leaf
          auto best_leaf_cut =
              cuts.cuts(leaf)[node_match[leaf].best_cut[phase]];
          node_position lp = compute_gate_position(best_leaf_cut);
          double wire = std::abs(np.x_coordinate - lp.x_coordinate) +
                        std::abs(np.y_coordinate - lp.y_coordinate);

          node_match[leaf].required_wirelength[phase] =
              std::min(node_match[leaf].required_wirelength[phase],
                       node_data.required_wirelength[use_phase] - wire);
          std::cout << "x is : " << lp.x_coordinate
                    << "y is : " << lp.y_coordinate << "\n";
          ++ctr;
        }
      }

      if (!node_data.same_match && node_data.map_refs[other_phase] > 0) {
        auto ctr = 0u;
        auto best_cut = cuts.cuts(index)[node_data.best_cut[other_phase]];
        // compute gate position of root gate
        node_position np = match_position[index];
        auto const& supergate = node_data.best_supergate[other_phase];
        for (auto leaf : best_cut) {
          auto phase = (node_data.phase[other_phase] >> ctr) & 1;
          // compute gate position of leaf
          auto best_leaf_cut =
              cuts.cuts(leaf)[node_match[leaf].best_cut[phase]];
          node_position lp = match_position[leaf];
          std::cout << "x is : " << lp.x_coordinate
                    << "y is : " << lp.y_coordinate << "\n";
          double wire = std::abs(np.x_coordinate - lp.x_coordinate) +
                        std::abs(np.y_coordinate - lp.y_coordinate);
          node_match[leaf].required_wirelength[phase] =
              std::min(node_match[leaf].required_wirelength[phase],
                       node_data.required_wirelength[other_phase] - wire);
          ++ctr;
        }
      }
    }
  }

  template <bool DO_AREA, bool MCTS = false>
  void match_phase(node<Ntk> const& n, uint8_t phase) {
    auto index = ntk.node_to_index(n);
    auto& node_data = node_match[index];
    if constexpr (MCTS) {
      /* if the phase of this node has been matched, skip */
      if (node_data.set_flag[phase]) return;
    }

    double best_arrival = std::numeric_limits<double>::max();
    double best_area_flow = std::numeric_limits<double>::max();
    float best_area = std::numeric_limits<float>::max();
    uint32_t best_size = UINT32_MAX;
    uint8_t best_cut = 0u;
    uint8_t best_phase = 0u;
    uint8_t cut_index = 0u;
    auto& cut_matches = matches[index];
    supergate<NInputs> const* best_supergate = node_data.best_supergate[phase];

    if (best_supergate != nullptr) {
      auto const& cut = cuts.cuts(index)[node_data.best_cut[phase]];

      best_phase = node_data.phase[phase];
      best_arrival = 0.0f;
      best_area_flow = best_supergate->area + cut_leaves_flow(cut, n, phase);
      best_area = best_supergate->area;
      best_cut = node_data.best_cut[phase];
      best_size = cut.size();

      auto ctr = 0u;
      for (auto l : cut) {
        double arrival_pin = node_match[l].arrival[(best_phase >> ctr) & 1] +
                             best_supergate->tdelay[ctr];
        best_arrival = std::max(best_arrival, arrival_pin);
        ++ctr;
      }
    }
    for (auto& cut : cuts.cuts(index)) {
      /* trivial cuts or not matched cuts */
      if ((*cut)->data.ignore) {
        ++cut_index;
        continue;
      }

      auto const& supergates = cut_matches[(*cut)->data.match_index].supergates;
      auto const negation =
          cut_matches[(*cut)->data.match_index].negations[phase];

      if (supergates[phase] == nullptr) {
        ++cut_index;
        continue;
      }

      /* match each gate and take the best one */
      for (auto const& gate : *supergates[phase]) {
        uint8_t gate_polarity = gate.polarity ^ negation;
        node_data.phase[phase] = gate_polarity;
        double area_local = gate.area + cut_leaves_flow(*cut, n, phase);
        double worst_arrival = 0.0f;

        auto ctr = 0u;
        for (auto l : *cut) {
          double arrival_pin =
              node_match[l].arrival[(gate_polarity >> ctr) & 1] +
              gate.tdelay[ctr];
          worst_arrival = std::max(worst_arrival, arrival_pin);
          ++ctr;
        }

        if constexpr (DO_AREA) {
          if (worst_arrival > node_data.required[phase] + epsilon) continue;
        }

        // std::cout << "compare map\n";
        if (compare_map<DO_AREA>(worst_arrival, best_arrival, area_local,
                                 best_area_flow, cut->size(), best_size)) {
          best_arrival = worst_arrival;
          best_area_flow = area_local;
          best_size = cut->size();
          best_cut = cut_index;
          best_area = gate.area;
          best_phase = gate_polarity;
          best_supergate = &gate;
        }
      }
      ++cut_index;
    }
    node_data.flows[phase] = best_area_flow;
    node_data.arrival[phase] = best_arrival;
    node_data.area[phase] = best_area;
    node_data.best_cut[phase] = best_cut;
    node_data.phase[phase] = best_phase;
    node_data.best_supergate[phase] = best_supergate;
  }

  template <bool SwitchActivity>
  void match_phase_exact(node<Ntk> const& n, uint8_t phase) {
    double best_arrival = std::numeric_limits<double>::max();
    float best_exact_area = std::numeric_limits<float>::max();
    float best_area = std::numeric_limits<float>::max();
    uint32_t best_size = UINT32_MAX;
    uint8_t best_cut = 0u;
    uint8_t best_phase = 0u;
    uint8_t cut_index = 0u;
    auto index = ntk.node_to_index(n);

    auto& node_data = node_match[index];
    auto& cut_matches = matches[index];
    supergate<NInputs> const* best_supergate = node_data.best_supergate[phase];

    /* recompute best match info */
    if (best_supergate != nullptr) {
      auto const& cut = cuts.cuts(index)[node_data.best_cut[phase]];

      best_phase = node_data.phase[phase];
      best_arrival = 0.0f;
      best_area = best_supergate->area;
      best_cut = node_data.best_cut[phase];
      best_size = cut.size();

      auto ctr = 0u;
      for (auto l : cut) {
        double arrival_pin = node_match[l].arrival[(best_phase >> ctr) & 1] +
                             best_supergate->tdelay[ctr];
        best_arrival = std::max(best_arrival, arrival_pin);
        ++ctr;
      }

      /* if cut is implemented, remove it from the cover */
      if (!node_data.same_match && node_data.map_refs[phase]) {
        best_exact_area =
            cut_deref<SwitchActivity>(cuts.cuts(index)[best_cut], n, phase);
      } else {
        best_exact_area =
            cut_ref<SwitchActivity>(cuts.cuts(index)[best_cut], n, phase);
        cut_deref<SwitchActivity>(cuts.cuts(index)[best_cut], n, phase);
      }
    }

    /* foreach cut */
    for (auto& cut : cuts.cuts(index)) {
      /* trivial cuts or not matched cuts */
      if ((*cut)->data.ignore) {
        ++cut_index;
        continue;
      }

      auto const& supergates = cut_matches[(*cut)->data.match_index].supergates;
      auto const negation =
          cut_matches[(*cut)->data.match_index].negations[phase];

      if (supergates[phase] == nullptr) {
        ++cut_index;
        continue;
      }

      /* match each gate and take the best one */
      for (auto const& gate : *supergates[phase]) {
        uint8_t gate_polarity = gate.polarity ^ negation;
        node_data.phase[phase] = gate_polarity;
        node_data.area[phase] = gate.area;
        float area_exact = cut_ref<SwitchActivity>(*cut, n, phase);
        cut_deref<SwitchActivity>(*cut, n, phase);
        double worst_arrival = 0.0f;

        auto ctr = 0u;
        for (auto l : *cut) {
          double arrival_pin =
              node_match[l].arrival[(gate_polarity >> ctr) & 1] +
              gate.tdelay[ctr];
          worst_arrival = std::max(worst_arrival, arrival_pin);
          ++ctr;
        }

        if (worst_arrival > node_data.required[phase] + epsilon) continue;

        if (compare_map<true>(worst_arrival, best_arrival, area_exact,
                              best_exact_area, cut->size(), best_size)) {
          best_arrival = worst_arrival;
          best_exact_area = area_exact;
          best_area = gate.area;
          best_size = cut->size();
          best_cut = cut_index;
          best_phase = gate_polarity;
          best_supergate = &gate;
        }
      }

      ++cut_index;
    }

    node_data.flows[phase] = best_exact_area;
    node_data.arrival[phase] = best_arrival;
    node_data.area[phase] = best_area;
    node_data.best_cut[phase] = best_cut;
    node_data.phase[phase] = best_phase;
    node_data.best_supergate[phase] = best_supergate;

    if (!node_data.same_match && node_data.map_refs[phase]) {
      best_exact_area =
          cut_ref<SwitchActivity>(cuts.cuts(index)[best_cut], n, phase);
    }
  }

  node_position compute_gate_position(cut_t const& cut) 
  {
    // std::cout<<"pin size of cut : "<<cut.pins.size()<<"\n";
    node_position gate_position;
    int crt = 0;
    for (auto& c : cut.pins) {
      if ( np[c].x_coordinate == 0 && np[c].y_coordinate )
      {
        std::cout<<"coordinate zero falut\n";
      }
      gate_position.x_coordinate += np[c].x_coordinate;
      gate_position.y_coordinate += np[c].y_coordinate;
      crt++;
    }
    gate_position.x_coordinate = gate_position.x_coordinate / crt;
    gate_position.y_coordinate = gate_position.y_coordinate / crt;
    return gate_position;
  }

  template <bool DO_WIRE, bool DO_DELAY>
  void match_wirelength(node<Ntk> const& n, uint8_t phase) {
    std::cout << "match wirelength\n";
    double best_arrival = std::numeric_limits<double>::max();
    double best_area_flow = std::numeric_limits<double>::max();
    float best_area = std::numeric_limits<float>::max();
    double best_wirelength = std::numeric_limits<double>::max();
    double best_total_wirelength = std::numeric_limits<double>::max();
    node_position best_gate_position;
    uint32_t best_size = UINT32_MAX;
    uint8_t best_cut = 0u;
    uint8_t best_phase = 0u;
    uint8_t cut_index = 0u;
    auto index = ntk.node_to_index(n);

    auto& node_data = node_match[index];
    auto& cut_matches = matches[index];
    supergate<NInputs> const* best_supergate = node_data.best_supergate[phase];

    /* recompute best match info */
    if (best_supergate != nullptr) {
      auto const& cut = cuts.cuts(index)[node_data.best_cut[phase]];
      best_phase = node_data.phase[phase];
      best_arrival = 0.0f;
      best_area_flow = best_supergate->area + cut_leaves_flow(cut, n, phase);
      best_area = best_supergate->area;
      best_cut = node_data.best_cut[phase];
      best_size = cut.size();

      auto ctr = 0u;
      for (auto l : cut) {
        double arrival_pin = node_match[l].arrival[(best_phase >> ctr) & 1] +
                             best_supergate->tdelay[ctr];
        best_arrival = std::max(best_arrival, arrival_pin);
        ++ctr;
      }
      node_position gate_position = compute_gate_position(cut);
      best_wirelength =
          compute_match_wirelength(cut, gate_position, best_phase);
      best_total_wirelength =
          compute_match_total_wirelength(cut, gate_position, best_phase);
    }

    /* foreach cut */
    std::cout << "start to match cut\n";
    for (auto& cut : cuts.cuts(index)) {
      /* trivial cuts or not matched cuts */
      if ((*cut)->data.ignore) {
        ++cut_index;
        continue;
      }
      std::cout << "supergate\n";
      auto const& supergates = cut_matches[(*cut)->data.match_index].supergates;
      auto const negation =
          cut_matches[(*cut)->data.match_index].negations[phase];

      if (supergates[phase] == nullptr) {
        ++cut_index;
        continue;
      }

      node_position gate_position = compute_gate_position(*cut);

      /* match each gate and take the best one */
      std::cout << "compute wirelength\n";
      for (auto const& gate : *supergates[phase]) {
        uint8_t gate_polarity = gate.polarity ^ negation;
        node_data.phase[phase] = gate_polarity;
        double area_local = gate.area + cut_leaves_flow(*cut, n, phase);
        double worst_arrival = 0.0f;
        double worst_wirelength =
            compute_match_wirelength(*cut, gate_position, best_phase);
        double worst_total_wirelength =
            compute_match_total_wirelength(*cut, gate_position, best_phase);

        auto ctr = 0u;
        std::cout << "delay computing\n";
        for (auto l : *cut) {
          double arrival_pin =
              node_match[l].arrival[(gate_polarity >> ctr) & 1] +
              gate.tdelay[ctr];
          worst_arrival = std::max(worst_arrival, arrival_pin);
          ++ctr;
        }

        if constexpr (DO_WIRE) {
          if (worst_wirelength > node_data.wirelength[phase] + epsilon)
            continue;
        }
        if constexpr (DO_DELAY) {
          if (worst_arrival > node_data.required[phase] + epsilon) continue;
        }

        std::cout << "comparing wirelength\n";
        if (compare_map_wirelength<DO_WIRE>(
                worst_wirelength, best_wirelength, worst_arrival, best_arrival,
                worst_total_wirelength, best_total_wirelength, cut->size(),
                best_size)) {
          best_wirelength = worst_wirelength;
          best_total_wirelength = worst_total_wirelength;
          best_arrival = worst_arrival;
          best_area_flow = area_local;
          best_size = cut->size();
          best_cut = cut_index;
          best_area = gate.area;
          best_phase = gate_polarity;
          best_supergate = &gate;
          best_gate_position.x_coordinate = gate_position.x_coordinate;
          best_gate_position.y_coordinate = gate_position.y_coordinate;
        }
      }

      ++cut_index;
    }

    node_data.wirelength[phase] = best_wirelength;
    node_data.total_wirelength[phase] = best_total_wirelength;
    node_data.flows[phase] = best_area_flow;
    node_data.arrival[phase] = best_arrival;
    node_data.area[phase] = best_area;
    node_data.best_cut[phase] = best_cut;
    node_data.phase[phase] = best_phase;
    node_data.best_supergate[phase] = best_supergate;
    match_position[index] = best_gate_position;
  }

  template <bool DO_WIRE, bool DO_DELAY, bool DO_AREA>
  void match_wirelength_balance(node<Ntk> const& n, uint8_t phase) {
    double best_arrival = std::numeric_limits<double>::max();
    double best_area_flow = std::numeric_limits<double>::max();
    float best_area = std::numeric_limits<float>::max();
    double best_wirelength = std::numeric_limits<double>::max();
    double best_total_wirelength = std::numeric_limits<double>::max();
    node_position best_gate_position;
    uint32_t best_size = UINT32_MAX;
    uint8_t best_cut = 0u;
    uint8_t best_phase = 0u;
    uint8_t cut_index = 0u;
    auto index = ntk.node_to_index(n);

    auto& node_data = node_match[index];
    auto& cut_matches = matches[index];
    supergate<NInputs> const* best_supergate = node_data.best_supergate[phase];

    /* recompute best match info */
    if (best_supergate != nullptr) {
      auto const& cut = cuts.cuts(index)[node_data.best_cut[phase]];
      best_phase = node_data.phase[phase];
      best_arrival = 0.0f;
      best_area_flow = best_supergate->area + cut_leaves_flow(cut, n, phase);
      best_area = best_supergate->area;
      best_cut = node_data.best_cut[phase];
      best_size = cut.size();

      auto ctr = 0u;
      for (auto l : cut) {
        double arrival_pin = node_match[l].arrival[(best_phase >> ctr) & 1] +
                             best_supergate->tdelay[ctr];
        best_arrival = std::max(best_arrival, arrival_pin);
        ++ctr;
      }
      node_position gate_position = compute_gate_position(cut);
      best_wirelength =
          compute_match_wirelength(cut, gate_position, best_phase);
      best_total_wirelength =
          compute_match_total_wirelength(cut, gate_position, best_phase);
    }

    /* foreach cut */
    for (auto& cut : cuts.cuts(index)) {
      /* trivial cuts or not matched cuts */
      if ((*cut)->data.ignore) {
        ++cut_index;
        continue;
      }

      auto const& supergates = cut_matches[(*cut)->data.match_index].supergates;
      auto const negation =
          cut_matches[(*cut)->data.match_index].negations[phase];

      if (supergates[phase] == nullptr) {
        ++cut_index;
        continue;
      }

      node_position gate_position = compute_gate_position(*cut);

      double worst_wirelength =
          compute_match_wirelength(*cut, gate_position, best_phase);

      double worst_total_wirelength =
          compute_match_total_wirelength(*cut, gate_position, best_phase);

      /* match each gate and take the best one */
      for (auto const& gate : *supergates[phase]) {
        uint8_t gate_polarity = gate.polarity ^ negation;
        node_data.phase[phase] = gate_polarity;
        double area_local = gate.area + cut_leaves_flow(*cut, n, phase);
        double worst_arrival = 0.0f;

        auto ctr = 0u;
        for (auto l : *cut) {
          double arrival_pin =
              node_match[l].arrival[(gate_polarity >> ctr) & 1] +
              gate.tdelay[ctr];
          worst_arrival = std::max(worst_arrival, arrival_pin);
          ++ctr;
        }

        if (DO_AREA) {
          if constexpr (DO_WIRE) {
            if (worst_wirelength >
                1.3 * node_data.required_wirelength[phase] + epsilon)
              continue;
            if (worst_arrival > 1.0 * node_data.required[phase] + epsilon)
              continue;
          }
        }
        if constexpr (DO_DELAY) {
          if (worst_arrival > 1.0 * node_data.required[phase] + epsilon)
            continue;
        }

        if constexpr (DO_AREA) {
          if (compare_map<DO_WIRE>(
                  area_local / area + worst_total_wirelength / total_wirelength,
                  best_area_flow / area +
                      best_total_wirelength / total_wirelength,
                  worst_wirelength, best_wirelength, cut->size(), best_size)) {
            best_wirelength = worst_wirelength;
            best_total_wirelength = worst_total_wirelength;
            best_arrival = worst_arrival;
            best_area_flow = area_local;
            best_size = cut->size();
            best_cut = cut_index;
            best_area = gate.area;
            best_phase = gate_polarity;
            best_supergate = &gate;
            best_gate_position.x_coordinate = gate_position.x_coordinate;
            best_gate_position.y_coordinate = gate_position.y_coordinate;
          } else {
            if (compare_map_wirelength<DO_WIRE>(
                    worst_wirelength, best_wirelength, worst_arrival,
                    best_arrival, worst_total_wirelength, best_total_wirelength,
                    cut->size(), best_size)) {
              best_wirelength = worst_wirelength;
              best_total_wirelength = worst_total_wirelength;
              best_arrival = worst_arrival;
              best_area_flow = area_local;
              best_size = cut->size();
              best_cut = cut_index;
              best_area = gate.area;
              best_phase = gate_polarity;
              best_supergate = &gate;
              best_gate_position.x_coordinate = gate_position.x_coordinate;
              best_gate_position.y_coordinate = gate_position.y_coordinate;
            }
          }
        }
      }
      ++cut_index;
    }

    node_data.wirelength[phase] = best_wirelength;
    node_data.total_wirelength[phase] = best_total_wirelength;
    node_data.flows[phase] = best_area_flow;
    node_data.arrival[phase] = best_arrival;
    node_data.area[phase] = best_area;
    node_data.best_cut[phase] = best_cut;
    node_data.phase[phase] = best_phase;
    node_data.best_supergate[phase] = best_supergate;
    match_position[index] = best_gate_position;
  }

  template <bool DO_AREA, bool ELA>
  void match_drop_phase(node<Ntk> const& n, float required_margin_factor) {
    auto index = ntk.node_to_index(n);
    auto& node_data = node_match[index];

    /* compute arrival adding an inverter to the other match phase */
    double worst_arrival_npos = node_data.arrival[1] + lib_inv_delay;
    double worst_arrival_nneg = node_data.arrival[0] + lib_inv_delay;
    bool use_zero = false;
    bool use_one = false;

    /* only one phase is matched */
    if (node_data.best_supergate[0] == nullptr) {
      set_match_complemented_phase(index, 1, worst_arrival_npos);
      if constexpr (ELA) {
        if (node_data.map_refs[2])
          cut_ref<false>(cuts.cuts(index)[node_data.best_cut[1]], n, 1);
      }
      return;
    } else if (node_data.best_supergate[1] == nullptr) {
      set_match_complemented_phase(index, 0, worst_arrival_nneg);
      if constexpr (ELA) {
        if (node_data.map_refs[2])
          cut_ref<false>(cuts.cuts(index)[node_data.best_cut[0]], n, 0);
      }
      return;
    }

    /* try to use only one match to cover both phases */
    if constexpr (!DO_AREA) {
      /* if arrival improves matching the other phase and inserting an inverter
       */
      if (worst_arrival_npos < node_data.arrival[0] + epsilon) {
        use_one = true;
      }
      if (worst_arrival_nneg < node_data.arrival[1] + epsilon) {
        use_zero = true;
      }
    } else {
      /* check if both phases + inverter meet the required time */
      use_zero = worst_arrival_nneg < (node_data.required[1] + epsilon -
                                       required_margin_factor * lib_inv_delay);
      use_one = worst_arrival_npos < (node_data.required[0] + epsilon -
                                      required_margin_factor * lib_inv_delay);
    }

    /* condition on not used phases, evaluate a substitution during exact area
     * recovery */
    if constexpr (ELA) {
      if (iteration != 0) {
        if (node_data.map_refs[0] == 0 || node_data.map_refs[1] == 0) {
          /* select the used match */
          auto phase = 0;
          auto nphase = 0;
          if (node_data.map_refs[0] == 0) {
            phase = 1;
            use_one = true;
            use_zero = false;
          } else {
            nphase = 1;
            use_one = false;
            use_zero = true;
          }
          /* select the not used match instead if it leads to area improvement
           * and doesn't violate the required time */
          if (node_data.arrival[nphase] + lib_inv_delay <
              node_data.required[phase] + epsilon) {
            auto size_phase =
                cuts.cuts(index)[node_data.best_cut[phase]].size();
            auto size_nphase =
                cuts.cuts(index)[node_data.best_cut[nphase]].size();

            if (compare_map<DO_AREA>(node_data.arrival[nphase] + lib_inv_delay,
                                     node_data.arrival[phase],
                                     node_data.flows[nphase] + lib_inv_area,
                                     node_data.flows[phase], size_nphase,
                                     size_phase)) {
              /* invert the choice */
              use_zero = !use_zero;
              use_one = !use_one;
            }
          }
        }
      }
    }

    if (!use_zero && !use_one) {
      /* use both phases */
      node_data.flows[0] = node_data.flows[0] / node_data.est_refs[0];
      node_data.flows[1] = node_data.flows[1] / node_data.est_refs[1];
      node_data.flows[2] = node_data.flows[0] + node_data.flows[1];
      node_data.same_match = false;
      return;
    }

    /* use area flow as a tiebreaker */
    if (use_zero && use_one) {
      auto size_zero = cuts.cuts(index)[node_data.best_cut[0]].size();
      auto size_one = cuts.cuts(index)[node_data.best_cut[1]].size();
      if (compare_map<DO_AREA>(worst_arrival_nneg, worst_arrival_npos,
                               node_data.flows[0], node_data.flows[1],
                               size_zero, size_one))
        use_one = false;
      else
        use_zero = false;
    }

    if (use_zero) {
      if constexpr (ELA) {
        /* set cut references */
        if (!node_data.same_match) {
          /* dereference the negative phase cut if in use */
          if (node_data.map_refs[1] > 0)
            cut_deref<false>(cuts.cuts(index)[node_data.best_cut[1]], n, 1);
          /* reference the positive cut if not in use before */
          if (node_data.map_refs[0] == 0 && node_data.map_refs[2])
            cut_ref<false>(cuts.cuts(index)[node_data.best_cut[0]], n, 0);
        } else if (node_data.map_refs[2])
          cut_ref<false>(cuts.cuts(index)[node_data.best_cut[0]], n, 0);
      }
      set_match_complemented_phase(index, 0, worst_arrival_nneg);
    } else {
      if constexpr (ELA) {
        /* set cut references */
        if (!node_data.same_match) {
          /* dereference the positive phase cut if in use */
          if (node_data.map_refs[0] > 0)
            cut_deref<false>(cuts.cuts(index)[node_data.best_cut[0]], n, 0);
          /* reference the negative cut if not in use before */
          if (node_data.map_refs[1] == 0 && node_data.map_refs[2])
            cut_ref<false>(cuts.cuts(index)[node_data.best_cut[1]], n, 1);
        } else if (node_data.map_refs[2])
          cut_ref<false>(cuts.cuts(index)[node_data.best_cut[1]], n, 1);
      }
      set_match_complemented_phase(index, 1, worst_arrival_npos);
    }
  }

  inline void set_match_complemented_phase(uint32_t index, uint8_t phase,
                                           double worst_arrival_n) {
    auto& node_data = node_match[index];
    auto phase_n = phase ^ 1;
    node_data.same_match = true;
    node_data.best_supergate[phase_n] = nullptr;
    node_data.best_cut[phase_n] = node_data.best_cut[phase];
    node_data.phase[phase_n] = node_data.phase[phase];
    node_data.arrival[phase_n] = worst_arrival_n;
    node_data.area[phase_n] = node_data.area[phase];
    node_data.flows[phase] = node_data.flows[phase] / node_data.est_refs[2];
    node_data.flows[phase_n] = node_data.flows[phase];
    node_data.flows[2] = node_data.flows[phase];
  }

  void match_constants(uint32_t index) {
    auto& node_data = node_match[index];

    kitty::static_truth_table<6> zero_tt;
    auto const supergates_zero = library.get_supergates(zero_tt);
    auto const supergates_one = library.get_supergates(~zero_tt);

    /* Not available in the library */
    if (supergates_zero == nullptr && supergates_one == nullptr) {
      return;
    }
    /* if only one is available, the other is obtained using an inverter */
    if (supergates_zero != nullptr) {
      node_data.best_supergate[0] = &((*supergates_zero)[0]);
      node_data.arrival[0] = node_data.best_supergate[0]->tdelay[0];
      node_data.area[0] = node_data.best_supergate[0]->area;
      node_data.phase[0] = 0;
    }
    if (supergates_one != nullptr) {
      node_data.best_supergate[1] = &((*supergates_one)[0]);
      node_data.arrival[1] = node_data.best_supergate[1]->tdelay[0];
      node_data.area[1] = node_data.best_supergate[1]->area;
      node_data.phase[1] = 0;
    } else {
      node_data.same_match = true;
      node_data.arrival[1] = node_data.arrival[0] + lib_inv_delay;
      node_data.area[1] = node_data.area[0] + lib_inv_area;
      node_data.phase[1] = 1;
    }
    if (supergates_zero == nullptr) {
      node_data.same_match = true;
      node_data.arrival[0] = node_data.arrival[1] + lib_inv_delay;
      node_data.area[0] = node_data.area[1] + lib_inv_area;
      node_data.phase[0] = 1;
    }
  }

  inline double cut_leaves_flow(cut_t const& cut, node<Ntk> const& n,
                                uint8_t phase) {
    double flow{0.0f};
    auto const& node_data = node_match[ntk.node_to_index(n)];

    uint8_t ctr = 0u;
    for (auto leaf : cut) {
      uint8_t leaf_phase = (node_data.phase[phase] >> ctr++) & 1;
      flow += node_match[leaf].flows[leaf_phase];
    }

    return flow;
  }

  template <bool SwitchActivity>
  float cut_ref(cut_t const& cut, node<Ntk> const& n, uint8_t phase) {
    auto const& node_data = node_match[ntk.node_to_index(n)];
    float count;

    if constexpr (SwitchActivity)
      count = switch_activity[ntk.node_to_index(n)];
    else
      count = node_data.area[phase];

    uint8_t ctr = 0;
    for (auto leaf : cut) {
      /* compute leaf phase using the current gate */
      uint8_t leaf_phase = (node_data.phase[phase] >> ctr++) & 1;

      if (ntk.is_constant(ntk.index_to_node(leaf))) {
        continue;
      } else if (ntk.is_ci(ntk.index_to_node(leaf))) {
        /* reference PIs, add inverter cost for negative phase */
        if (leaf_phase == 1u) {
          if (node_match[leaf].map_refs[1]++ == 0u) {
            if constexpr (SwitchActivity)
              count += switch_activity[leaf];
            else
              count += lib_inv_area;
          }
        } else {
          ++node_match[leaf].map_refs[0];
        }
        continue;
      }

      if (node_match[leaf].same_match) {
        /* Add inverter area if not present yet and leaf node is implemented in
         * the opposite phase */
        if (node_match[leaf].map_refs[leaf_phase]++ == 0u &&
            node_match[leaf].best_supergate[leaf_phase] == nullptr) {
          if constexpr (SwitchActivity)
            count += switch_activity[leaf];
          else
            count += lib_inv_area;
        }
        /* Recursive referencing if leaf was not referenced */
        if (node_match[leaf].map_refs[2]++ == 0u) {
          count += cut_ref<SwitchActivity>(
              cuts.cuts(leaf)[node_match[leaf].best_cut[leaf_phase]],
              ntk.index_to_node(leaf), leaf_phase);
        }
      } else {
        ++node_match[leaf].map_refs[2];
        if (node_match[leaf].map_refs[leaf_phase]++ == 0u) {
          count += cut_ref<SwitchActivity>(
              cuts.cuts(leaf)[node_match[leaf].best_cut[leaf_phase]],
              ntk.index_to_node(leaf), leaf_phase);
        }
      }
    }
    return count;
  }

  template <bool SwitchActivity>
  float cut_deref(cut_t const& cut, node<Ntk> const& n, uint8_t phase) {
    auto const& node_data = node_match[ntk.node_to_index(n)];
    float count;

    if constexpr (SwitchActivity)
      count = switch_activity[ntk.node_to_index(n)];
    else
      count = node_data.area[phase];

    uint8_t ctr = 0;
    for (auto leaf : cut) {
      /* compute leaf phase using the current gate */
      uint8_t leaf_phase = (node_data.phase[phase] >> ctr++) & 1;

      if (ntk.is_constant(ntk.index_to_node(leaf))) {
        continue;
      } else if (ntk.is_ci(ntk.index_to_node(leaf))) {
        /* dereference PIs, add inverter cost for negative phase */
        if (leaf_phase == 1u) {
          if (--node_match[leaf].map_refs[1] == 0u) {
            if constexpr (SwitchActivity)
              count += switch_activity[leaf];
            else
              count += lib_inv_area;
          }
        } else {
          --node_match[leaf].map_refs[0];
        }
        continue;
      }

      if (node_match[leaf].same_match) {
        /* Add inverter area if it is used only by the current gate and leaf
         * node is implemented in the opposite phase */
        if (--node_match[leaf].map_refs[leaf_phase] == 0u &&
            node_match[leaf].best_supergate[leaf_phase] == nullptr) {
          if constexpr (SwitchActivity)
            count += switch_activity[leaf];
          else
            count += lib_inv_area;
        }
        /* Recursive dereferencing */
        if (--node_match[leaf].map_refs[2] == 0u) {
          count += cut_deref<SwitchActivity>(
              cuts.cuts(leaf)[node_match[leaf].best_cut[leaf_phase]],
              ntk.index_to_node(leaf), leaf_phase);
        }
      } else {
        --node_match[leaf].map_refs[2];
        if (--node_match[leaf].map_refs[leaf_phase] == 0u) {
          count += cut_deref<SwitchActivity>(
              cuts.cuts(leaf)[node_match[leaf].best_cut[leaf_phase]],
              ntk.index_to_node(leaf), leaf_phase);
        }
      }
    }
    return count;
  }

  void insert_buffers() {
    if (lib_buf_id != UINT32_MAX) {
      double area_old = area;
      bool buffers = false;

      ntk.foreach_co([&](auto const& f) {
        auto const& n = ntk.get_node(f);
        if (!ntk.is_constant(n) && ntk.is_ci(n) && !ntk.is_complemented(f)) {
          area += lib_buf_area;
          delay = std::max(delay, node_match[ntk.node_to_index(n)].arrival[0] +
                                      lib_inv_delay);
          buffers = true;
        }
      });

      /* round stats */
      if (ps.verbose && buffers) {
        std::stringstream stats{};
        float area_gain = 0.0f;

        area_gain = float((area_old - area) / area_old * 100);

        stats << fmt::format(
            "[i] Buffering: Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n",
            delay, area, area_gain);
        st.round_stats.push_back(stats.str());
      }
    }
  }

  std::pair<map_ntk_t, klut_map> initialize_map_network() {
    map_ntk_t dest(library.get_gates());
    klut_map old2new;

    old2new[ntk.node_to_index(ntk.get_node(ntk.get_constant(false)))][0] =
        dest.get_constant(false);
    old2new[ntk.node_to_index(ntk.get_node(ntk.get_constant(false)))][1] =
        dest.get_constant(true);

    ntk.foreach_pi([&](auto const& n) {
      old2new[ntk.node_to_index(n)][0] = dest.create_pi();
      res2ntk[old2new[ntk.node_to_index(n)][0]] =
          index_phase_pair{ntk.node_to_index(n), 0};
    });

    return {dest, old2new};
  }

  std::pair<seq_map_ntk_t, klut_map> initialize_map_seq_network() {
    seq_map_ntk_t dest(library.get_gates());
    klut_map old2new;

    old2new[ntk.node_to_index(ntk.get_node(ntk.get_constant(false)))][0] =
        dest.get_constant(false);
    old2new[ntk.node_to_index(ntk.get_node(ntk.get_constant(false)))][1] =
        dest.get_constant(true);

    ntk.foreach_pi([&](auto const& n) {
      old2new[ntk.node_to_index(n)][0] = dest.create_pi();
    });
    ntk.foreach_ro([&](auto const& n) {
      old2new[ntk.node_to_index(n)][0] = dest.create_ro();
    });

    return {dest, old2new};
  }

  template <class NtkDest>
  void finalize_cover(NtkDest& res, klut_map& old2new) {
    for (auto const& n : top_order) {
      auto index = ntk.node_to_index(n);
      auto const& node_data = node_match[index];

      /* add inverter at PI if needed */
      if (ntk.is_constant(n)) {
        if (node_data.best_supergate[0] == nullptr &&
            node_data.best_supergate[1] == nullptr)
          continue;
      } else if (ntk.is_ci(n)) {
        if (node_data.map_refs[1] > 0) {
          old2new[index][1] = res.create_not(old2new[n][0]);
          res2ntk[old2new[index][1]] = index_phase_pair{index, 1};
          res.add_binding(res.get_node(old2new[index][1]), lib_inv_id);
          // record the fanin of inverter
          res2ntk[old2new[index][1]].is_inverter = true;
          res2ntk[old2new[index][1]].pins[1] = old2new[index][0];
        }
        continue;
      }

      /* continue if cut is not in the cover */
      if (node_data.map_refs[2] == 0u) continue;

      unsigned phase = (node_data.best_supergate[0] != nullptr) ? 0 : 1;

      /* add used cut */
      if (node_data.same_match || node_data.map_refs[phase] > 0) {
        create_lut_for_gate<NtkDest>(res, old2new, index, phase);

        /* add inverted version if used */
        if (node_data.same_match && node_data.map_refs[phase ^ 1] > 0) {
          old2new[index][phase ^ 1] = res.create_not(old2new[index][phase]);
          res.add_binding(res.get_node(old2new[index][phase ^ 1]), lib_inv_id);
          res2ntk[old2new[index][phase ^ 1]] = index_phase_pair{index, phase ^ 1};
          // record the fanin of inverter
          res2ntk[old2new[index][phase ^ 1]].is_inverter = true;
          res2ntk[old2new[index][phase ^ 1]].pins[1] = old2new[index][phase];
        }
      }

      phase = phase ^ 1;
      /* add the optional other match if used */
      if (!node_data.same_match && node_data.map_refs[phase] > 0) {
        create_lut_for_gate<NtkDest>(res, old2new, index, phase);
      }
    }

    /* create POs */
    ntk.foreach_po([&](auto const& f) {
      if (ntk.is_complemented(f)) {
        res.create_po(old2new[ntk.node_to_index(ntk.get_node(f))][1]);
      } else if (!ntk.is_constant(ntk.get_node(f)) &&
                 ntk.is_ci(ntk.get_node(f)) && lib_buf_id != UINT32_MAX) {
        /* create buffers for POs */
        static uint64_t _buf = 0x2;
        kitty::dynamic_truth_table tt_buf(1);
        kitty::create_from_words(tt_buf, &_buf, &_buf + 1);
        const auto buf = res.create_node(
            {old2new[ntk.node_to_index(ntk.get_node(f))][0]}, tt_buf);
        res.create_po(buf);
        res.add_binding(res.get_node(buf), lib_buf_id);
      } else {
        res.create_po(old2new[ntk.node_to_index(ntk.get_node(f))][0]);
      }
    });

    if constexpr (has_foreach_ri_v<Ntk>) {
      ntk.foreach_ri([&](auto const& f) {
        if (ntk.is_complemented(f)) {
          res.create_ri(old2new[ntk.node_to_index(ntk.get_node(f))][1]);
        } else if (!ntk.is_constant(ntk.get_node(f)) &&
                   ntk.is_ci(ntk.get_node(f)) && lib_buf_id != UINT32_MAX) {
          /* create buffers for RIs */
          static uint64_t _buf = 0x2;
          kitty::dynamic_truth_table tt_buf(1);
          kitty::create_from_words(tt_buf, &_buf, &_buf + 1);
          const auto buf = res.create_node(
              {old2new[ntk.node_to_index(ntk.get_node(f))][0]}, tt_buf);
          res.create_ri(buf);
          res.add_binding(res.get_node(buf), lib_buf_id);
        } else {
          res.create_ri(old2new[ntk.node_to_index(ntk.get_node(f))][0]);
        }
      });
    }

    /* write final results */
    st.area = area;
    st.delay = delay;
    st.wirelength = wirelength;
    st.total_wirelength = total_wirelength;
    if (ps.eswp_rounds) st.power = compute_switching_power();
  }

  // testing
  void print_node_children(signal<klut_network> s,
                           std::vector<signal<klut_network>> children) {
    std::cout << "for node " << s << " its child is ";
    for (auto& i : children) {
      std::cout << i << " ";
    }
    std::cout << "\n";
  }

  void print_pins() {
    for (int i = 0; i < ntk.size(); i++) {
      for (auto& cut : cuts.cuts(i)) {
        for (auto& pin : cut->pins) {
          std::cout << "pins: " << pin << ", ";
        }
        std::cout << "\n";
      }
    }
  }

  template <class NtkDest>
  void create_lut_for_gate(NtkDest& res, klut_map& old2new, uint32_t index,
                           unsigned phase) {
    auto const& node_data = node_match[index];
    auto& best_cut = cuts.cuts(index)[node_data.best_cut[phase]];
    auto const& gate = node_data.best_supergate[phase]->root;

    /* permutate and negate to obtain the matched gate truth table */
    std::vector<signal<klut_network>> children(gate->num_vars);

    auto ctr = 0u;
    for (auto l : best_cut) {
      if (ctr >= gate->num_vars) break;
      children[node_data.best_supergate[phase]->permutation[ctr]] =
          old2new[l][(node_data.phase[phase] >> ctr) & 1];
      ++ctr;
    }

    if (!gate->is_super) {
      /* create the node */
      auto f = res.create_node(children, gate->function);
      res.add_binding(res.get_node(f), gate->root->id);

      /* if one of the children is inverter, add the new node to its fanout pin */
      for (auto const& child : children)
      {
        if ( res2ntk[child].is_inverter )
        {
          assert(res2ntk[child].pins[1] > 0 && "fanout of inverter has been set");
          res2ntk[child].pins[1] = f;
        }
      }

      /* add the node in the data structure */
      old2new[index][phase] = f;
      res2ntk[f] = index_phase_pair{index, phase};
      // print_node_children(f, children);
    } else {
      /* supergate, create sub-gates */
      auto f = create_lut_for_gate_rec<NtkDest>(res, *gate, children);
      old2new[index][phase] = f;
      res2ntk[f] = index_phase_pair{index, phase};
    }
  }

  template <class NtkDest>
  signal<klut_network> create_lut_for_gate_rec(
      NtkDest& res, composed_gate<NInputs> const& gate,
      std::vector<signal<klut_network>> const& children) {
    std::vector<signal<klut_network>> children_local(gate.fanin.size());

    auto i = 0u;
    for (auto const fanin : gate.fanin) {
      if (fanin->root == nullptr) {
        /* terminal condition */
        children_local[i] = children[fanin->id];
      } else {
        children_local[i] =
            create_lut_for_gate_rec<NtkDest>(res, *fanin, children);
      }
      ++i;
    }

    auto f = res.create_node(children_local, gate.root->function);
    res.add_binding(res.get_node(f), gate.root->id);
    return f;
  }

  template <bool DO_AREA>
  inline bool compare_map(double arrival, double best_arrival, double area_flow,
                          double best_area_flow, uint32_t size,
                          uint32_t best_size) {
    if constexpr (DO_AREA) {
      if (area_flow < best_area_flow - epsilon) {
        return true;
      } else if (area_flow > best_area_flow + epsilon) {
        return false;
      } else if (arrival < best_arrival - epsilon) {
        return true;
      } else if (arrival > best_arrival + epsilon) {
        return false;
      }
    } else {
      if (arrival < best_arrival - epsilon) {
        return true;
      } else if (arrival > best_arrival + epsilon) {
        return false;
      } else if (area_flow < best_area_flow - epsilon) {
        return true;
      } else if (area_flow > best_area_flow + epsilon) {
        return false;
      }
    }
    if (size < best_size) {
      return true;
    }
    return false;
  }

  template <bool DO_AREA>
  inline bool compare_map_wirelength(double wirelength, double best_wirelength,
                                     double arrival, double best_arrival,
                                     double total_wirelength,
                                     double best_total_wirelength,
                                     uint32_t size, uint32_t best_size) {
    if constexpr (DO_AREA) {
      if (weight_w_d(total_wirelength, arrival) <
          weight_w_d(best_total_wirelength, best_arrival) - epsilon) {
        return true;
      } else if (weight_w_d(total_wirelength, arrival) >
                 weight_w_d(best_total_wirelength, best_arrival) + epsilon) {
        return false;
      } else if (weight_w_d(wirelength, arrival) <
                 weight_w_d(best_wirelength, best_arrival) - epsilon) {
        return true;
      } else if (weight_w_d(wirelength, arrival) >
                 weight_w_d(best_wirelength, best_arrival) + epsilon) {
        return false;
      }
    } else {
      if (weight_w_d(wirelength, arrival) <
          weight_w_d(best_wirelength, best_arrival) - epsilon) {
        return true;
      } else if (weight_w_d(wirelength, arrival) >
                 weight_w_d(best_wirelength, best_arrival) + epsilon) {
        return false;
      } else if (weight_w_d(total_wirelength, arrival) <
                 weight_w_d(best_total_wirelength, best_arrival) - epsilon) {
        return true;
      } else if (weight_w_d(total_wirelength, arrival) >
                 weight_w_d(best_total_wirelength, best_arrival) + epsilon) {
        return false;
      }
    }
    if (size < best_size) return true;
    return false;
  }

  double compute_switching_power() {
    double power = 0.0f;

    for (auto const& n : top_order) {
      const auto index = ntk.node_to_index(n);
      auto& node_data = node_match[index];

      if (ntk.is_constant(n)) {
        if (node_data.best_supergate[0] == nullptr &&
            node_data.best_supergate[1] == nullptr)
          continue;
      } else if (ntk.is_ci(n)) {
        if (node_data.map_refs[1] > 0)
          power += switch_activity[ntk.node_to_index(n)];
        continue;
      }

      /* continue if cut is not in the cover */
      if (node_match[index].map_refs[2] == 0u) continue;

      unsigned phase = (node_data.best_supergate[0] != nullptr) ? 0 : 1;

      if (node_data.same_match || node_data.map_refs[phase] > 0) {
        power += switch_activity[ntk.node_to_index(n)];

        if (node_data.same_match && node_data.map_refs[phase ^ 1] > 0)
          power += switch_activity[ntk.node_to_index(n)];
      }

      phase = phase ^ 1;
      if (!node_data.same_match && node_data.map_refs[phase] > 0) {
        power += switch_activity[ntk.node_to_index(n)];
      }
    }

    return power;
  }

  double compute_match_wirelength(cut_t const& cut,
                                  node_position const& gate_position,
                                  uint8_t best_phase) {
    double worst_wl = 0.0f;
    int ctr = 0;
    for (auto& c : cut) {
      double wl_temp = std::abs(gate_position.x_coordinate -
                                match_position[c].x_coordinate) +
                       std::abs(gate_position.y_coordinate -
                                match_position[c].y_coordinate) +
                       node_match[c].wirelength[(best_phase >> ctr) & 1];
      worst_wl = std::max(wl_temp, worst_wl);
      ctr++;
    }
    return worst_wl;
  }

  double compute_match_total_wirelength(cut_t const& cut,
                                        node_position const& gate_position,
                                        uint8_t best_phase) {
    double worst_wl = 0.0f;
    int ctr = 0;
    for (auto& c : cut) {
      double wl_temp = std::abs(gate_position.x_coordinate -
                                match_position[c].x_coordinate) +
                       std::abs(gate_position.y_coordinate -
                                match_position[c].y_coordinate) +
                       node_match[c].total_wirelength[(best_phase >> ctr) & 1];
      auto& pin_data = node_match[c];
      worst_wl += wl_temp / pin_data.est_refs[2];
      ctr++;
    }
    return worst_wl;
  }

  double compute_wirelength(cut_t const& cut) {
    node_position roots, leafs;
    int count_l = 0, count_r = 0;
    for (auto r : cut.pins) {
      roots.x_coordinate += np[r].x_coordinate;
      roots.y_coordinate += np[r].y_coordinate;
      count_r++;
    }
    for (auto l : cut) {
      leafs.x_coordinate += np[l].x_coordinate;
      leafs.y_coordinate += np[l].y_coordinate;
      count_l++;
    }
    roots.x_coordinate = roots.x_coordinate / count_r;
    roots.y_coordinate = roots.y_coordinate / count_r;
    leafs.x_coordinate = leafs.x_coordinate / count_l;
    leafs.y_coordinate = leafs.y_coordinate / count_l;

    /* wirelength_pin computation */
    double distance = std::abs(roots.x_coordinate - leafs.x_coordinate) +
                      std::abs(roots.y_coordinate - leafs.y_coordinate);
    return distance;
  }

  double weight_w_d(double wirelength_t, double delay_t) {
    return 0.8 * wirelength_t + 0.2 * delay_t / delay;
  }

  void print_node_match() {
    for (auto i = 0u; i < ntk.size(); ++i) {
      if (ntk.is_constant(i) || ntk.is_ci(i)) continue;
      std::cout << "i = " << i << std::endl;
      std::cout << "arrival: " << node_match[i].arrival[0] << ", "
                << node_match[i].arrival[1] << std::endl;
      std::cout << "required: " << node_match[i].required[0] << ", "
                << node_match[i].required[1] << std::endl;
      std::cout << "area: " << node_match[i].area[0] << ", "
                << node_match[i].area[1] << std::endl;
      std::cout << "wirelength: " << node_match[i].wirelength[0] << ", "
                << node_match[i].wirelength[1] << std::endl;
      std::cout << "map_refs: " << node_match[i].map_refs[0] << ", "
                << node_match[i].map_refs[1] << ", "
                << node_match[i].map_refs[2] << std::endl;
      std::cout << "est_refs: " << node_match[i].est_refs[0] << ", "
                << node_match[i].est_refs[1] << ", "
                << node_match[i].est_refs[2] << std::endl;
      std::cout << "flows: " << node_match[i].flows[0] << ", "
                << node_match[i].flows[1] << ", " << node_match[i].flows[2]
                << std::endl
                << std::endl;
    }
    std::cout << "area, delay, wirelength, iteration: " << area << ", " << delay
              << ", " << wirelength << ", " << iteration << std::endl
              << std::endl;
  }

 public:
  Ntk const& ntk;
  std::string lib_file = "library.lib";
  tech_library<NInputs, Configuration> const& library;
  tech_library<NInputs, Configuration> const& cri_lib; /* lib contains only AND, NAND and INV gates */
  std::vector<node_position> np;
  map_params const& ps;
  map_stats& st;

  uint32_t iteration{0};         /* current mapping iteration */
  double delay{0.0f};            /* current delay of the mapping */
  double area{0.0f};             /* current area of the mapping */
  double wirelength{0.0f};       /* current wirelength of the mapping */
  double total_wirelength{0.0f}; /* current total wirelength of the mapping */
  const float epsilon{0.005f};   /* epsilon */
  size_t queue_size{0};             /* size of multiset */
  uint32_t set_nodes{0};         /* current amount of set nodes */

  /* lib inverter info */
  float lib_inv_area;
  float lib_inv_delay;
  uint32_t lib_inv_id;

  /* lib buffer info */
  float lib_buf_area;
  float lib_buf_delay;
  uint32_t lib_buf_id;

  std::vector<node<Ntk>> top_order;
  std::vector<node_match_tech<NInputs>> node_match;
  match_map matches;
  match_map unmapped_matches;
  std::vector<float> switch_activity;
  network_cuts_t cuts;
  network_cuts_t cuts_unmapped;
  std::vector<node_position> match_position;
  std::unordered_map<signal<klut_network>, index_phase_pair> res2ntk;

  /* variable for incremental mapping */
  double _reward {0};
  double _mulReward {0};
  map_ntk_t eventual_res;
  bool _terminal { false };

  /* file for saving result */
  std::string best_result_file;
  std::string best_mylResult_file;
  std::string result_file = "";

  // statistic queue
  std::vector<index_cut_supergate> vec_ics;
  std::multiset<index_cut_supergate, index_cut_supergate::CompareArea> area_queue;
  std::multiset<index_cut_supergate, index_cut_supergate::CompareDelay> delay_queue;
  std::multiset<index_cut_supergate, index_cut_supergate::CompareWirelength> wirelength_queue;
  std::multiset<index_cut_supergate,
                index_cut_supergate::CompareTotalWirelength>
      totalwirelength_queue;
};

} /* namespace detail */

/*! \brief Technology mapping.
 *
 * This function implements a technology mapping algorithm. It is controlled by
 * a template argument `CutData` (defaulted to `cut_enumeration_tech_map_cut`).
 * The argument is similar to the `CutData` argument in `cut_enumeration`, which
 * can specialize the cost function to select priority cuts and store additional
 * data. The default argument gives priority firstly to the cut size, then
 * delay, and lastly to area flow. Thus, it is more suited for delay-oriented
 * mapping. The type passed as `CutData` must implement the following four
 * fields:
 *
 * - `uint32_t delay`
 * - `float flow`
 * - `uint8_t match_index`
 * - `bool ignore`
 *
 * See
 * `include/mockturtle/algorithms/cut_enumeration/cut_enumeration_tech_map_cut.hpp`
 * for one example of a CutData type that implements the cost function that is
 * used in the technology mapper.
 *
 * The function takes the size of the cuts in the template parameter `CutSize`.
 *
 * The function returns a k-LUT network. Each LUT abstracts a gate of the
 * technology library.
 *
 * **Required network functions:**
 * - `size`
 * - `is_ci`
 * - `is_constant`
 * - `node_to_index`
 * - `index_to_node`
 * - `get_node`
 * - `foreach_pi`
 * - `foreach_po`
 * - `foreach_co`
 * - `foreach_node`
 * - `fanout_size`
 *
 * \param ntk Network
 * \param library Technology library
 * \param ps Mapping params
 * \param pst Mapping statistics
 *
 * The implementation of this algorithm was inspired by the
 * mapping command ``map`` in ABC.
 */
using namespace MCTS;

template <class Ntk, unsigned CutSize = 5u,
          typename CutData = cut_enumeration_tech_map_cut, unsigned NInputs,
          classification_type Configuration>
void map_mcts(
    Ntk const& ntk, std::string lib_file,
    tech_library<NInputs, Configuration> const& library,
    tech_library<NInputs, Configuration> const& cri_lib,
    std::vector<node_position> const& np, map_params const& ps = {},
    map_stats* pst = nullptr) {
  static_assert(is_network_type_v<Ntk>, "Ntk is not a network type");
  static_assert(has_size_v<Ntk>, "Ntk does not implement the size method");
  static_assert(has_is_ci_v<Ntk>, "Ntk does not implement the is_ci method");
  static_assert(has_is_constant_v<Ntk>,
                "Ntk does not implement the is_constant method");
  static_assert(has_node_to_index_v<Ntk>,
                "Ntk does not implement the node_to_index method");
  static_assert(has_index_to_node_v<Ntk>,
                "Ntk does not implement the index_to_node method");
  static_assert(has_get_node_v<Ntk>,
                "Ntk does not implement the get_node method");
  static_assert(has_foreach_pi_v<Ntk>,
                "Ntk does not implement the foreach_pi method");
  static_assert(has_foreach_po_v<Ntk>,
                "Ntk does not implement the foreach_po method");
  static_assert(has_foreach_node_v<Ntk>,
                "Ntk does not implement the foreach_node method");
  static_assert(has_fanout_size_v<Ntk>,
                "Ntk does not implement the fanout_size method");

  map_stats st;
  std::cout<<"tech_incre_map_impl<Ntk, CutSize, CutData, NInputs, Configuration>\n";
  detail::tech_incre_map_impl<Ntk, CutSize, CutData, NInputs, Configuration> p(
      ntk, lib_file, library, cri_lib, np, ps, st);
  // auto res = p.run();

  MCTS_params mcts_param;
  MCTS_impl mct(p, mcts_param);
  mct.run();
  auto impl_prt = mct.get();

  std::cout<<"the result's binding networks size = "<<impl_prt->eventual_res.size()<<"\n";

  st.time_total = st.time_mapping + st.cut_enumeration_st.time_total;
  if (ps.verbose && !st.mapping_error) st.report();
  if (pst) *pst = st;
}
}  // namespace mockturtle