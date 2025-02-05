#pragma once

#include <iostream>
#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/algorithms/cut_enumeration.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/views/binding_view.hpp>
#include <mockturtle/utils/tech_library.hpp>
#include "../core/RUDY.hpp"

using namespace mockturtle;

namespace phyLS {

template<class Ntk, unsigned CutSize, typename CutData, unsigned NInputs, classification_type Configuration, typename CongestionMap>
class phy_map_impl : public mockturtle::detail::tech_map_impl<Ntk, CutSize, CutData, NInputs, Configuration>
{
public:
  using parent = mockturtle::detail::tech_map_impl<Ntk, CutSize, CutData, NInputs, Configuration>;
  
  explicit phy_map_impl(Ntk const& ntk, 
                       tech_library<NInputs, Configuration> const& library,
                       map_params const& ps, 
                       map_stats& st)
      : parent(ntk, library, ps, st)
  {
  }

  explicit phy_map_impl(Ntk const& ntk,
                       tech_library<NInputs, Configuration> const& library,
                       std::vector<float> const& switch_activity,
                       map_params const& ps,
                       map_stats& st)
      : parent(ntk, library, switch_activity, ps, st)
  {
  }

  explicit phy_map_impl(Ntk const& ntk,
                       tech_library<NInputs, Configuration> const& library,
                       std::vector<node_position> const& np,
                       map_params const& ps,
                       map_stats& st)
      : parent(ntk, library, np, ps, st)
  {
  }


private:
  void setCongestionMap(std::vector<node_position> placement, binding_view<klut_network> const& res)
  {
    _congestion_map = CongestionMap(&placement, &res, res.num_pis(), res.num_pos());
    _congestion_map.calculateRudy();
  }

  template <bool DO_TOTALWIRE, bool DO_POWER, bool DO_TRADE>
  void matchCongWire(node<Ntk> const& n, uint8_t phase) {
    double best_arrival = std::numeric_limits<double>::max();
    double best_area_flow = std::numeric_limits<double>::max();
    float best_area = std::numeric_limits<float>::max();
    double best_wirelength = std::numeric_limits<double>::max();
    double best_total_wirelength = std::numeric_limits<double>::max();
    node_position best_gate_position;
    bool best_position = false;
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
      node_position gate_position;
      if (cut.size() != 1) {
        gate_position = compute_gate_position(cut);
      } else {
        gate_position = match_position[index];
      }
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

      node_position gate_position;
      if ((*cut).size() != 1) {
        gate_position = compute_gate_position(*cut);
      } else {
        gate_position = match_position[index];
      }

      /* match each gate and take the best one */
      for (auto const& gate : *supergates[phase]) {
        uint8_t gate_polarity = gate.polarity ^ negation;
        node_data.phase[phase] = gate_polarity;
        double area_local = gate.area + cut_leaves_flow(*cut, n, phase);
        double worst_arrival = 0.0f;
        double worst_wirelength =
            compute_match_wirelength(*cut, gate_position, gate_polarity);
        double worst_total_wirelength =
            compute_match_total_wirelength(*cut, gate_position, gate_polarity);

        auto ctr = 0u;
        for (auto l : *cut) {
          double arrival_pin =
              node_match[l].arrival[(gate_polarity >> ctr) & 1] +
              gate.tdelay[ctr];
          worst_arrival = std::max(worst_arrival, arrival_pin);
          ++ctr;
        }

        if (worst_arrival > node_data.required[phase] + epsilon) continue;

        if (!DO_TRADE) {
          if constexpr (!DO_POWER) {
            if constexpr (DO_TOTALWIRE) {
              if (worst_wirelength > node_data.wirelength[phase] + epsilon)
                continue;
            }
          }
        }

        if (compare_map_wirelength<DO_TOTALWIRE, DO_TRADE>(
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
          best_position = true;
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
    if (best_position){
      match_position[index] = best_gate_position;
    } 
  }


  // member variables
  CongestionMap _congestion_map;
};

template <class Ntk, unsigned CutSize = 5u,
          typename CutData = cut_enumeration_tech_map_cut, unsigned NInputs,
          classification_type Configuration>
binding_view<klut_network> phymap(
    Ntk const& ntk, tech_library<NInputs, Configuration> const& library,
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
  phy_map_impl<Ntk, CutSize, CutData, NInputs, Configuration, RUDY<Ntk>> p(
      ntk, library, np, ps, st);
  auto res = p.run();

  st.time_total = st.time_mapping + st.cut_enumeration_st.time_total;
  if (ps.verbose && !st.mapping_error) st.report();

  if (pst) *pst = st;

  return res;
}

} // namespace phyLS