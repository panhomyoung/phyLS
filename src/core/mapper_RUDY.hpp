#pragma once

#include <memory>

#include <mockturtle/algorithms/mapper.hpp>
#include "../core/RUDY.hpp"
#include "../core/utils/data_structure.hpp"


namespace mockturtle {



namespace detail {
template<class Ntk, unsigned CutSize, typename CutData, unsigned NInputs, classification_type Configuration>
class phy_map_impl
{
public:
  using network_cuts_t = fast_network_cuts<Ntk, CutSize, true, CutData>;
  using cut_t = typename network_cuts_t::cut_t;
  using match_map = std::unordered_map<uint32_t, std::vector<cut_match_tech<NInputs>>>;
  using klut_map = std::unordered_map<uint32_t, std::array<signal<klut_network>, 2>>;
  using map_ntk_t = binding_view<klut_network>;
  using seq_map_ntk_t = binding_view<sequential<klut_network>>;

  struct node_match_phy : public detail::node_match_tech<NInputs> {
    double congest_flow[2];
    double congestion[2];
    node_position position[2] {node_position{0, 0}, node_position{0, 0}};
  };

public:
  explicit phy_map_impl( Ntk const& ntk, tech_library<NInputs, Configuration> const& library, map_params const& ps, map_stats& st )
      : ntk( ntk ),
        library( library ),
        ps( ps ),
        st( st ),
        node_match( ntk.size() ),
        matches(),
        switch_activity( ps.eswp_rounds ? switching_activity( ntk, ps.switching_activity_patterns ) : std::vector<float>( 0 ) ),
        cuts( fast_cut_enumeration<Ntk, CutSize, true, CutData>( ntk, ps.cut_enumeration_ps, &st.cut_enumeration_st ) ),
        _binding_roots(ntk.size())
  {
    std::tie( lib_inv_area, lib_inv_delay, lib_inv_id ) = library.get_inverter_info();
    std::tie( lib_buf_area, lib_buf_delay, lib_buf_id ) = library.get_buffer_info();
  }

  explicit phy_map_impl( Ntk const& ntk, tech_library<NInputs, Configuration> const& library, std::vector<float> const& switch_activity, map_params const& ps, map_stats& st )
      : ntk( ntk ),
        library( library ),
        ps( ps ),
        st( st ),
        node_match( ntk.size() ),
        matches(),
        switch_activity( switch_activity ),
        cuts( fast_cut_enumeration<Ntk, NInputs, true, CutData>( ntk, ps.cut_enumeration_ps, &st.cut_enumeration_st ) ),
        _binding_roots(ntk.size())
  {
    std::tie( lib_inv_area, lib_inv_delay, lib_inv_id ) = library.get_inverter_info();
    std::tie( lib_buf_area, lib_buf_delay, lib_buf_id ) = library.get_buffer_info();
  }

  explicit phy_map_impl(Ntk const& ntk, tech_library<NInputs, Configuration> const& library, std::vector<node_position> const& np, map_params const& ps, map_stats& st)
      : ntk(ntk),
        library(library),
        np(np),
        ps(ps),
        st(st),
        node_match(ntk.size()),
        matches(),
        switch_activity( ps.eswp_rounds ? switching_activity(ntk, ps.switching_activity_patterns) : std::vector<float>(0)),
        cuts(fast_cut_enumeration<Ntk, CutSize, true, CutData>( ntk, ps.cut_enumeration_ps, &st.cut_enumeration_st)),
        _binding_roots(ntk.size())
  {
    std::tie(lib_inv_area, lib_inv_delay, lib_inv_id) = library.get_inverter_info();
    std::tie(lib_buf_area, lib_buf_delay, lib_buf_id) = library.get_buffer_info();
  }

  map_ntk_t run()
  {
    stopwatch t( st.time_mapping );

    auto [res, old2new] = initialize_map_network();

    /* compute and save topological order */
    top_order.reserve( ntk.size() );
    topo_view<Ntk>( ntk ).foreach_node( [this]( auto n ) {
      top_order.push_back( n );
    } );

    /* match cuts with gates */
    compute_matches();

    /* init the data structure */
    init_nodes();

    /* execute mapping */
    switch (ps.strategy) {
      case map_params::def:
        if (!execute_mapping()) return res;
        break;
      case map_params::area:
        if (!execute_area_mapping()) return res;
        break;
      case map_params::delay:
        if (!execute_delay_mapping()) return res;
        break;
      case map_params::performance:
        if (!execute_wirelength_mapping<false, false>()) return res;
        break;
      case map_params::power:
        if (!execute_wirelength_mapping<true, false>()) return res;
        break;
      case map_params::balance:
        if (!execute_wirelength_mapping<true, true>()) return res;
        break;
    }
    /* insert buffers for POs driven by PIs */
    insert_buffers();

    /* generate the output network */
    finalize_cover<map_ntk_t>( res, old2new );

    return res;
  }

  seq_map_ntk_t run_seq()
  {
    stopwatch t( st.time_mapping );

    auto [res, old2new] = initialize_map_seq_network();

    /* compute and save topological order */
    top_order.reserve( ntk.size() );
    topo_view<Ntk>( ntk ).foreach_node( [this]( auto n ) {
      top_order.push_back( n );
    } );

    /* match cuts with gates */
    compute_matches();

    /* init the data structure */
    init_nodes();

    /* execute mapping */
    if ( !execute_mapping() )
      return res;

    /* insert buffers for POs driven by PIs */
    insert_buffers();

    /* generate the output network */
    finalize_cover<seq_map_ntk_t>( res, old2new );

    return res;
  }

  map_ntk_t rudy_map_test() {
    auto [res, old2new] = initialize_map_network();

    /* compute and save topological order */
    top_order.reserve(ntk.size());
    topo_view<Ntk>(ntk).foreach_node([this](auto n) {
      top_order.push_back(n);
    });

    /* match cuts with gates */
    compute_matches();

    ntk.foreach_node([&](auto const& n) {
      auto index = ntk.node_to_index(n);
      auto& node_data = node_match[index];
      node_data.position[0] = np[index];
      node_data.position[1] = np[index];
    });

    ntk.foreach_node([&](auto const& n) {
      auto index = ntk.node_to_index(n);
      auto& node_data = node_match[index];
      std::cout << "Node " << index << " has position " << node_data.position[0].x_coordinate << " " << node_data.position[0].y_coordinate << std::endl;
    });

    /* init the data structure */
    init_nodes();

    /* execute mapping */
    bool success = compute_mapping<false>();
    if (!success) {
      std::cout << "Failed to map the network" << std::endl;
      return res;
    } 
    compute_required_time();

    success = compute_mapping<true>();
    if (!success) {
      std::cout << "Failed to map the network" << std::endl;
      return res;
    }
    compute_required_time();

    success = compute_mapping_wirelength<true, false, false>();
    if (!success) {
      std::cout << "Failed to map the network" << std::endl;
      return res;
    }
    compute_required_time();

    /* generate the output network */
    auto [res_temp, old2new_temp] = initialize_map_network<true>();

    std::cout << "second time printing position of the nodes" << std::endl;
    ntk.foreach_node([&](auto const& n) {
      auto index = ntk.node_to_index(n);
      auto& node_data = node_match[index];
      std::cout << "Node " << index << " has position " << node_data.position[0].x_coordinate << " " << node_data.position[0].y_coordinate << std::endl;
    });

    finalize_cover_congest<map_ntk_t, true>(res_temp, old2new_temp);

    // get_binding_position(match_position);
    // TODO: Implement the position of the nodes
    set_RUDY_map(&match_position, &res_temp, res.num_pis(), res.num_pos());
    calculateRUDY();

    // rudy_map->show();
    
    insert_buffers();

    // ntk.foreach_node([&](auto const& n) {
    //   auto index = ntk.node_to_index(n);
    //   auto& node_data = node_match[index];
    //   std::cout << "Node " << index << " has position " << node_data.position[0].x_coordinate << " " << node_data.position[0].y_coordinate << std::endl;
    // });

    /* generate the output network */
    finalize_cover<map_ntk_t>(res, old2new);

    return res;
  }

protected:
  void set_RUDY_map(std::vector<node_position>* placement_p, map_ntk_t* binding_network_p, uint32_t num_pis, uint32_t num_pos) {
    rudy_map = std::make_unique<phyLS::RUDY<map_ntk_t>>(placement_p, binding_network_p, static_cast<int>(num_pis), static_cast<int>(num_pos));
  }

  void calculateRUDY() {
    rudy_map->calculateRudy();
  }

  bool execute_mapping()
  {
    /* compute mapping for delay */
    if ( !ps.skip_delay_round )
    {
      std::cout << "Compute mapping for delay" << std::endl;
      if ( !compute_mapping<false>() )
      {
        return false;
      }
    }
    /* compute mapping using global area flow */
    while ( iteration < ps.area_flow_rounds + 1 )
    {
      compute_required_time();
      if ( !compute_mapping<true>() )
      {
        return false;
      }
    }

    /* compute mapping using exact area */
    while ( iteration < ps.ela_rounds + ps.area_flow_rounds + 1 )
    {
      std::cout << "Compute mapping using exact area" << std::endl;
      compute_required_time();
      if ( !compute_mapping_exact<false>() )
      {
        return false;
      }
    }

    /* compute mapping using exact switching activity estimation */
    while ( iteration < ps.eswp_rounds + ps.ela_rounds + ps.area_flow_rounds + 1 )
    {
      std::cout << "Compute mapping using exact switching activity estimation" << std::endl;
      compute_required_time();
      if ( !compute_mapping_exact<true>() )
      {
        return false;
      }
    }

    return true;
  }

  template <bool DO_POWER, bool DO_TRADE>
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

    /* compute mapping for total wirelength */
    if (ps.wirelength_rounds) {
      compute_required_time();
      if (!compute_mapping_wirelength<false, DO_POWER, DO_TRADE>())
        return false;
    }

    /* compute mapping for total wirelength */
    if (iteration < ps.area_flow_rounds + ps.total_wirelength_rounds + 2) {
      compute_required_time();
      if (!compute_mapping_wirelength<true, DO_POWER, DO_TRADE>()) return false;
    }

    /* compute mapping using exact area */
    while (iteration < ps.ela_rounds + ps.area_flow_rounds + 2) {
      compute_required_time();
      if (!compute_mapping_exact<false>()) return false;
    }

    return true;
  }

  bool execute_delay_mapping() 
  {
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

  bool execute_area_mapping() 
  {
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

  void init_nodes()
  {
    ntk.foreach_node( [this]( auto const& n, auto ) {
      const auto index = ntk.node_to_index( n );
      auto& node_data = node_match[index];

      node_data.est_refs[0] = node_data.est_refs[1] = node_data.est_refs[2] = static_cast<float>( ntk.fanout_size( n ) );

      if ( ntk.is_constant( n ) )
      {
        /* all terminals have flow 1.0 */
        node_data.flows[0] = node_data.flows[1] = node_data.flows[2] = 0.0f;
        node_data.arrival[0] = node_data.arrival[1] = 0.0f;
        match_constants( index );
      }
      else if ( ntk.is_ci( n ) )
      {
        /* all terminals have flow 1.0 */
        node_data.flows[0] = node_data.flows[1] = node_data.flows[2] = 0.0f;
        node_data.arrival[0] = 0.0f;
        /* PIs have the negative phase implemented with an inverter */
        node_data.arrival[1] = lib_inv_delay;
      }
    } );
  }

  template <typename CutType>
  void search_nodes_pins(uint32_t n, CutType* cut, uint16_t level = 0) {
    if (cut->size() <= 1) return;
    if (level > 7)
    {
      std::cout << "cut size = " << cut->size() << " leaves = ";
      for (auto& i : cut->pins)
        std::cout << i << " ";
      std::cout << std::endl;
      return;
    }
    else if (ntk.is_ci(ntk.index_to_node(n)) || ntk.is_constant(ntk.index_to_node(n)))
    {
      return;
    }

    node_match[n].set_flag[0] = true;

    bool is_pins = false;

    ntk.foreach_fanin(ntk.index_to_node(n), [&](signal<Ntk> const& fi) {
      uint32_t child = ntk.node_to_index(ntk.get_node(fi));

      std::cout << "fanin[" << n << "] = " << child << std::endl;

      if (node_match[child].set_flag[0] == true) return;
      bool in_leaves = false;
      for (uint32_t& i : *cut) {
        if (child == i) {
          in_leaves = true;
          break;
        }
      }
      if (!in_leaves) {
        if (level > 5) {
          std::cerr << "Error: cut size in leaves = " << cut->size() << std::endl;
          return;
        }
        
        // Add the father into the nodes collection when the node is between the root and the leaves
        if (std::find(cut->nodes.begin(), cut->nodes.end(), n) != cut->nodes.end())
          std::cerr << "The node " << child << " has been already in the cut of " << n << std::endl;

        search_nodes_pins(child, cut, level + 1);
      } else {
        is_pins = true;
        if (std::find(cut->pins.begin(), cut->pins.end(), n) != cut->pins.end())
          std::cerr << "The node " << child << " has been already in the pins of " << n << std::endl;
        return;
      }
    });
    // std::cout << "Leaves of node " << n << " : ";

    cut->nodes.push_back(n);
    if (is_pins) 
    {
      cut->pins.push_back(n);
    }
    // std::cout << "Leaves of node " << n << " : ";

    if (level == 0)
    {
      // std::cout << "Leaves of node " << n << " : ";
      for (auto& i : *cut)
      {
        std::cout << i << " ";
      }
      std::cout << std::endl;

      if (std::find(cut->pins.begin(), cut->pins.end(), n) == cut->pins.end())
      {
        std::cout <<"Root node isn't in the pins of the cut" << std::endl;
        cut->pins.push_back(n);
      }

      if (std::find(cut->nodes.begin(), cut->nodes.end(), n) == cut->nodes.end())
      {
        std::cout << "Root node isn't in the nodes of the cut" << std::endl;
        cut->pins.push_back(n);
      }

      // std::cout << "nodes of node " << n << " : ";
      for (auto& i : cut->nodes)
      {
        // std::cout << i << " ";
        node_match[i].set_flag[0] = false;
      }
      std::cout << std::endl;
    }
  }

  template <typename CutType>
  void search_pins(uint32_t n, CutType* cut, uint16_t level = 0) 
  {
    if (cut->size() <= 1) return;
    if (level > 5)
    {
      std::cout << "cut size = " << cut->size() << " level = " ;
      for (auto& i : cut->pins)
        std::cout << i << " ";
      std::cout << std::endl;
      return;
    }
    else if (ntk.is_ci(ntk.index_to_node(n))) 
    {
      return;
    }

    if (level == 0) cut->pins.push_back(n);
    uint16_t level_ = level + 1;

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
    std::cout << "Ninputs = " << NInputs << std::endl;
    /* match gates */
    ntk.foreach_gate( [&]( auto const& n ) {
      const auto index = ntk.node_to_index( n );
      if (index == 131) std::cout << "Cut size of " << n << " = " << cuts.cuts( index ).size() << std::endl;
      std::vector<cut_match_tech<NInputs>> node_matches;

      auto i = 0u;
      for ( auto& cut : cuts.cuts( index ) )
      {
        if (index == 131) 
        {
          if (ntk.is_constant(index)) std::cout << "131 is constant" << std::endl;
          std::cout << "Cut size of " << n << " = " << cuts.cuts( index ).size() << std::endl;
          for (auto& i : *cut)
            std::cout << i << " ";
        }
        /* ignore unit cut */
        if ( cut->size() == 1 && *cut->begin() == index )
        {
          if (index == 131) std::cout << "ignoring for too small " << n << std::endl;
          ( *cut )->data.ignore = true;
          continue;
        }
        if ( cut->size() > NInputs )
        {
          if (index == 131) std::cout << "ignoring for too large " << n << std::endl;
          /* Ignore cuts too big to be mapped using the library */
          ( *cut )->data.ignore = true;
          continue;
        }
        const auto tt = cuts.truth_table( *cut );
        const auto fe = kitty::extend_to<6>( tt );
        auto fe_canon = fe;

        uint8_t negations_pos = 0;
        uint8_t negations_neg = 0;

        /* match positive polarity */
        if constexpr ( Configuration == classification_type::p_configurations )
        {
          auto canon = kitty::exact_n_canonization( fe );
          fe_canon = std::get<0>( canon );
          negations_pos = std::get<1>( canon );
        }
        auto const supergates_pos = library.get_supergates( fe_canon );

        /* match negative polarity */
        if constexpr ( Configuration == classification_type::p_configurations )
        {
          auto canon = kitty::exact_n_canonization( ~fe );
          fe_canon = std::get<0>( canon );
          negations_neg = std::get<1>( canon );
        }
        else
        {
          fe_canon = ~fe;
        }
        auto const supergates_neg = library.get_supergates( fe_canon );

        if ( supergates_pos != nullptr || supergates_neg != nullptr )
        {
          cut_match_tech<NInputs> match{ { supergates_pos, supergates_neg }, { negations_pos, negations_neg } };

          node_matches.push_back( match );
          ( *cut )->data.match_index = i++;
        }
        else
        {
          /* Ignore not matched cuts */
          ( *cut )->data.ignore = true;
        }
        search_nodes_pins(index, cut);
        // if (ps.strategy == map_params::performance ||
        //     ps.strategy == map_params::power ||
        //     ps.strategy == map_params::balance)
        //   search_pins(index, cut);
      }

      matches[index] = node_matches;
    } );
  }

  template<bool DO_AREA>
  bool compute_mapping()
  {
    for ( auto const& n : top_order )
    {
      if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
      {
        continue;
      }
      
      /* match positive phase */
      match_phase<DO_AREA>( n, 0u );

      /* match negative phase */
      match_phase<DO_AREA>( n, 1u );

      /* try to drop one phase */
      match_drop_phase<DO_AREA, false>( n, 0 );
    }

    double area_old = area;
    bool success = set_mapping_refs<false>();

    /* round stats */
    if ( ps.verbose )
    {
      std::stringstream stats{};
      float area_gain = 0.0f;

      if ( iteration != 1 )
        area_gain = float( ( area_old - area ) / area_old * 100 );

      if constexpr ( DO_AREA )
      {
        stats << fmt::format( "[i] AreaFlow : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n", delay, area, area_gain );
      }
      else
      {
        stats << fmt::format( "[i] Delay    : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n", delay, area, area_gain );
      }
      st.round_stats.push_back( stats.str() );
    }

    return success;
  }

  template<bool SwitchActivity>
  bool compute_mapping_exact()
  {
    for ( auto const& n : top_order )
    {
      if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
        continue;

      auto index = ntk.node_to_index( n );
      auto& node_data = node_match[index];

      /* recursively deselect the best cut shared between
       * the two phases if in use in the cover */
      if ( node_data.same_match && node_data.map_refs[2] != 0 )
      {
        if ( node_data.best_supergate[0] != nullptr )
          cut_deref<SwitchActivity>( cuts.cuts( index )[node_data.best_cut[0]], n, 0u );
        else
          cut_deref<SwitchActivity>( cuts.cuts( index )[node_data.best_cut[1]], n, 1u );
      }

      /* match positive phase */
      match_phase_exact<SwitchActivity>( n, 0u );

      /* match negative phase */
      match_phase_exact<SwitchActivity>( n, 1u );

      /* try to drop one phase */
      match_drop_phase<true, true>( n, 0 );
    }

    double area_old = area;
    bool success = set_mapping_refs<true>();

    /* round stats */
    if ( ps.verbose )
    {
      float area_gain = float( ( area_old - area ) / area_old * 100 );
      std::stringstream stats{};
      if constexpr ( SwitchActivity )
        stats << fmt::format( "[i] Switching: Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n", delay, area, area_gain );
      else
        stats << fmt::format( "[i] Area     : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n", delay, area, area_gain );
      st.round_stats.push_back( stats.str() );
    }

    return success;
  }

  std::pair<double, double> wireCongestCompute(node<Ntk> const& n, cut_t const& cut, node_position const& gate_position, 
                            uint8_t best_phase) {
    double wire_congest = 0.0f;
    double congest_flow = 0.0f;
    auto index = ntk.node_to_index(n);

    const bool is_temporary = true;
    int ctr = 0;
    for (auto& c : cut) {
      rudy_map->addRectRudy<is_temporary>(gate_position.x_coordinate, gate_position.y_coordinate, 
                                          node_match[c].position[(best_phase >> ctr) & 1].x_coordinate,
                                          node_match[c].position[(best_phase >> ctr) & 1].y_coordinate);
      ++ctr;
    }

    ctr = 0;
    for (auto& c : cut) {
      double congestion = rudy_map->hpwlCongestCompute(gate_position.x_coordinate, gate_position.y_coordinate,
                                                       node_match[c].position[(best_phase >> ctr) & 1].x_coordinate, 
                                                       node_match[c].position[(best_phase >> ctr) & 1].y_coordinate);
      wire_congest += congestion;
      congest_flow += congestion + node_match[c].wireCongest[best_phase >> ctr];
      ++ctr;
    }
    ctr = 0;
    for (auto& c : cut) {
      rudy_map->removeRectRudy<is_temporary>(gate_position.x_coordinate, gate_position.y_coordinate, 
                                          node_match[c].position[(best_phase >> ctr) & 1].x_coordinate,
                                          node_match[c].position[(best_phase >> ctr) & 1].y_coordinate);
      ++ctr;
    }

    return {wire_congest, congest_flow};
  }

  template <bool DO_TOTALWIRE, bool DO_POWER, bool DO_TRADE>
  void match_wireCongest(node<Ntk> const& n, uint8_t phase) {
    double best_arrival = std::numeric_limits<double>::max();
    double best_area_flow = std::numeric_limits<double>::max();
    float best_area = std::numeric_limits<float>::max();
    double best_wirelength = std::numeric_limits<double>::max();
    double best_total_wirelength = std::numeric_limits<double>::max();
    double best_wireCongest = std::numeric_limits<double>::max();
    double best_congest_flow = std::numeric_limits<double>::max();

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

    // Remove RUDY induced by the binding root of this node
    auto binding_roots = get_binding_roots(n, phase);
    for (auto& root : binding_roots) {
      rudy_map->nodeRUDYRemove(index);
    }

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
      
      if (cut.size() != 1) {
        best_gate_position = compute_gate_position(cut);
      } else {
        std::cerr << "Error: This cut has only one leaf" << std::endl;
        best_gate_position = node_data.position[phase];
      }
      best_wirelength =
          compute_match_wirelength(cut, best_gate_position, best_phase);
      best_total_wirelength =
          compute_match_total_wirelength(cut, best_gate_position, best_phase);
      best_congest_flow = 
          (wireCongestCompute(n, cut, best_gate_position, best_phase)).second;
      // Restore RUDY induced by the binding root of this node
    }

    for (auto const& cut : cuts.cuts(index)) {
      if (cut->data.ignore) {
        ++cut_index;
        continue;
      } else if (cut->size() <= 1) {
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
        std::cerr << "Error: This cut has only one leaf (in interation)" << std::endl;
        continue;
      }

      for (auto const& gate : *supergates[phase]) {
        uint8_t gate_polarity = gate.polarity ^ negation;
        node_data.phase[phase] = gate_polarity;
        double area_local = gate.area + cut_leaves_flow(*cut, n, phase);
        double worst_arrival = 0.0f;
        double worst_wirelength =
            compute_match_wirelength(*cut, gate_position, gate_polarity);
        double worst_total_wirelength =
            compute_match_total_wirelength(*cut, gate_position, gate_polarity);
        auto [worst_wire_congest, worst_congest_flow] = 
            wireCongestCompute(*cut, gate_position, gate_polarity);

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
          best_gate_position = gate_position;
          best_wireCongest = worst_wire_congest;
          best_congest_flow = worst_congest_flow;
          best_position = true;
        }
      }
      ++cut_index;
    }
    // Restore RUDY induced by the binding root of this node
    rudy_map->clearTempRUDY();

    node_data.wirelength[phase] = best_wirelength;
    node_data.total_wirelength[phase] = best_total_wirelength;
    node_data.flows[phase] = best_area_flow;
    node_data.arrival[phase] = best_arrival;
    node_data.area[phase] = best_area;
    node_data.best_cut[phase] = best_cut;
    node_data.phase[phase] = best_phase;
    node_data.best_supergate[phase] = best_supergate;
    node_data.position[phase] = best_gate_position;
    node_data.congestion[phase] = best_wireCongest;
    node_data.congest_flow[phase] = best_congest_flow;
  }

  template <bool DO_TOTALWIRE, bool DO_POWER>
  bool compute_wireCongest() {
    for (auto const& n : top_order) {
      if (ntk.is_constant(n) || ntk.is_ci(n)) continue;

      auto index = ntk.node_to_index(n);
      auto& node_data = node_match[index];

      match_wireCongest<DO_TOTALWIRE, DO_POWER>(n, 0u);

      /* match negative wire&delay phase */
      match_wireCongest<DO_TOTALWIRE, DO_POWER>(n, 1u);

      /* try to drop one delay phase */
      match_wirelength_drop_phase<DO_TOTALWIRE>(n);
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

  template <bool DO_TOTALWIRE, bool DO_POWER, bool DO_TRADE>
  bool compute_mapping_wirelength() {
    for (auto const& n : top_order) {
      uint32_t idx = ntk.node_to_index(n);
      if (ntk.is_constant(n) || ntk.is_ci(n)) continue;

      /* match positive wire&delay phase */
      match_wirelength<DO_TOTALWIRE, DO_POWER, DO_TRADE>(n, 0u);

      /* match negative wire&delay phase */
      match_wirelength<DO_TOTALWIRE, DO_POWER, DO_TRADE>(n, 1u);

      /* try to drop one delay phase */
      match_wirelength_drop_phase<DO_TOTALWIRE, DO_TRADE>(n);
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

  bool compute_mapping_wirelength_exact() {
    for (auto const& n : top_order) {
      if (ntk.is_constant(n) || ntk.is_ci(n)) continue;

      auto index = ntk.node_to_index(n);
      auto& node_data = node_match[index];

      /* recursively deselect the best cut shared between
       * the two phases if in use in the cover */
      if (node_data.same_match && node_data.map_refs[2] != 0) {
        if (node_data.best_supergate[0] != nullptr)
          cut_deref<false>(cuts.cuts(index)[node_data.best_cut[0]], n, 0u);
        else
          cut_deref<false>(cuts.cuts(index)[node_data.best_cut[1]], n, 1u);
      }

      /* match positive phase */
      match_wirelength_exact(n, 0u);

      /* match negative phase */
      match_wirelength_exact(n, 1u);

      /* try to drop one phase */
      match_drop_phase<true, true>(n, 0);
    }

    double area_old = area;
    bool success = set_mapping_refs_wirelength<true>();

    /* round stats */
    if (ps.verbose) {
      float area_gain = float((area_old - area) / area_old * 100);
      std::stringstream stats{};
      stats << fmt::format(
          "[i] Area     : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n",
          delay, area, area_gain);
      st.round_stats.push_back(stats.str());
    }

    return success;
  }

  template<bool ELA>
  bool set_mapping_refs()
  {
    const auto coef = 1.0f / ( 2.0f + ( iteration + 1 ) * ( iteration + 1 ) );

    if constexpr ( !ELA )
    {
      for ( auto i = 0u; i < node_match.size(); ++i )
      {
        node_match[i].map_refs[0] = node_match[i].map_refs[1] = node_match[i].map_refs[2] = 0u;
      }
    }

    /* compute the current worst delay and update the mapping refs */
    delay = 0.0f;
    ntk.foreach_co( [this]( auto s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );

      if (ntk.is_complemented(s)) {
        delay = std::max(delay, node_match[index].arrival[1]);
      } else {
        delay = std::max(delay, node_match[index].arrival[0]);
      }

      if constexpr ( !ELA )
      {
        node_match[index].map_refs[2]++;
        if ( ntk.is_complemented( s ) )
          node_match[index].map_refs[1]++;
        else
          node_match[index].map_refs[0]++;
      }
    } );

    /* compute current area and update mapping refs in top-down order */
    area = 0.0f;
    for ( auto it = top_order.rbegin(); it != top_order.rend(); ++it )
    {
      const auto index = ntk.node_to_index( *it );
      auto& node_data = node_match[index];

      /* skip constants and PIs */
      if ( ntk.is_constant( *it ) )
      {
        if ( node_match[index].map_refs[2] > 0u )
        {
          /* if used and not available in the library launch a mapping error */
          if ( node_data.best_supergate[0] == nullptr && node_data.best_supergate[1] == nullptr )
          {
            std::cerr << "[i] MAP ERROR: technology library does not contain constant gates, impossible to perform mapping" << std::endl;
            st.mapping_error = true;
            return false;
          }
        }
        continue;
      }
      else if ( ntk.is_ci( *it ) )
      {
        if ( node_match[index].map_refs[1] > 0u )
        {
          /* Add inverter area over the negated fanins */
          area += lib_inv_area;
        }
        continue;
      }

      /* continue if not referenced in the cover */
      if ( node_match[index].map_refs[2] == 0u )
        continue;

      unsigned use_phase = node_data.best_supergate[0] == nullptr ? 1u : 0u;

      if ( node_data.best_supergate[use_phase] == nullptr )
      {
        /* Library is not complete, mapping is not possible */
        std::cerr << "[i] MAP ERROR: technology library is not complete, impossible to perform mapping" << std::endl;
        st.mapping_error = true;
        return false;
      }

      if ( node_data.same_match || node_data.map_refs[use_phase] > 0 )
      {
        if constexpr ( !ELA )
        {
          auto const& best_cut = cuts.cuts( index )[node_data.best_cut[use_phase]];
          auto ctr = 0u;

          for ( auto const leaf : best_cut )
          {
            node_match[leaf].map_refs[2]++;
            if ( ( node_data.phase[use_phase] >> ctr++ ) & 1 )
              node_match[leaf].map_refs[1]++;
            else
              node_match[leaf].map_refs[0]++;
          }
        }
        area += node_data.area[use_phase];
        if ( node_data.same_match && node_data.map_refs[use_phase ^ 1] > 0 )
        {
          area += lib_inv_area;
        }
      }

      /* invert the phase */
      use_phase = use_phase ^ 1;

      /* if both phases are implemented and used */
      if ( !node_data.same_match && node_data.map_refs[use_phase] > 0 )
      {
        if constexpr ( !ELA )
        {
          auto const& best_cut = cuts.cuts( index )[node_data.best_cut[use_phase]];
          auto ctr = 0u;
          for ( auto const leaf : best_cut )
          {
            node_match[leaf].map_refs[2]++;
            if ( ( node_data.phase[use_phase] >> ctr++ ) & 1 )
              node_match[leaf].map_refs[1]++;
            else
              node_match[leaf].map_refs[0]++;
          }
        }
        area += node_data.area[use_phase];
      }
    }

    /* blend estimated references */
    for ( auto i = 0u; i < ntk.size(); ++i )
    {
      node_match[i].est_refs[2] = coef * node_match[i].est_refs[2] + ( 1.0f - coef ) * std::max( 1.0f, static_cast<float>( node_match[i].map_refs[2] ) );
      node_match[i].est_refs[1] = coef * node_match[i].est_refs[1] + ( 1.0f - coef ) * std::max( 1.0f, static_cast<float>( node_match[i].map_refs[1] ) );
      node_match[i].est_refs[0] = coef * node_match[i].est_refs[0] + ( 1.0f - coef ) * std::max( 1.0f, static_cast<float>( node_match[i].map_refs[0] ) );
    }

    ++iteration;
    return true;
  }

  template <bool ELA>
  bool set_mapping_refs_wirelength() 
  {
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
    congestion = 0.0f;
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
        congestion += node_data.congestion[use_phase];
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
        congestion += node_data.congestion[use_phase];
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

  void compute_required_time()
  {
    for ( auto i = 0u; i < node_match.size(); ++i )
    {
      node_match[i].required[0] = node_match[i].required[1] = std::numeric_limits<double>::max();
    }

    /* return in case of `skip_delay_round` */
    if ( iteration == 0 )
      return;

    auto required = delay;

    if ( ps.required_time != 0.0f )
    {
      /* Global target time constraint */
      if ( ps.required_time < delay - epsilon )
      {
        if ( !ps.skip_delay_round && iteration == 1 )
          std::cerr << fmt::format( "[i] MAP WARNING: cannot meet the target required time of {:.2f}", ps.required_time ) << std::endl;
      }
      else
      {
        required = ps.required_time;
      }
    }

    /* set the required time at POs */
    ntk.foreach_co( [&]( auto const& s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );
      if ( ntk.is_complemented( s ) )
        node_match[index].required[1] = required;
      else
        node_match[index].required[0] = required;
    } );

    /* propagate required time to the PIs */
    for ( auto it = top_order.rbegin(); it != top_order.rend(); ++it )
    {
      if ( ntk.is_ci( *it ) || ntk.is_constant( *it ) )
        break;

      const auto index = ntk.node_to_index( *it );

      if ( node_match[index].map_refs[2] == 0 )
        continue;

      auto& node_data = node_match[index];

      unsigned use_phase = node_data.best_supergate[0] == nullptr ? 1u : 0u;
      unsigned other_phase = use_phase ^ 1;

      assert( node_data.best_supergate[0] != nullptr || node_data.best_supergate[1] != nullptr );
      assert( node_data.map_refs[0] || node_data.map_refs[1] );

      /* propagate required time over the output inverter if present */
      if ( node_data.same_match && node_data.map_refs[other_phase] > 0 )
      {
        node_data.required[use_phase] = std::min( node_data.required[use_phase], node_data.required[other_phase] - lib_inv_delay );
      }

      if ( node_data.same_match || node_data.map_refs[use_phase] > 0 )
      {
        auto ctr = 0u;
        auto best_cut = cuts.cuts( index )[node_data.best_cut[use_phase]];
        auto const& supergate = node_data.best_supergate[use_phase];
        for ( auto leaf : best_cut )
        {
          auto phase = ( node_data.phase[use_phase] >> ctr ) & 1;
          node_match[leaf].required[phase] = std::min( node_match[leaf].required[phase], node_data.required[use_phase] - supergate->tdelay[ctr] );
          ++ctr;
        }
      }

      if ( !node_data.same_match && node_data.map_refs[other_phase] > 0 )
      {
        auto ctr = 0u;
        auto best_cut = cuts.cuts( index )[node_data.best_cut[other_phase]];
        auto const& supergate = node_data.best_supergate[other_phase];
        for ( auto leaf : best_cut )
        {
          auto phase = ( node_data.phase[other_phase] >> ctr ) & 1;
          node_match[leaf].required[phase] = std::min( node_match[leaf].required[phase], node_data.required[other_phase] - supergate->tdelay[ctr] );
          ++ctr;
        }
      }
    }
  }

  template<bool DO_AREA>
  void match_phase( node<Ntk> const& n, uint8_t phase )
  {
    double best_arrival = std::numeric_limits<double>::max();
    double best_area_flow = std::numeric_limits<double>::max();
    float best_area = std::numeric_limits<float>::max();
    uint32_t best_size = UINT32_MAX;
    uint8_t best_cut = 0u;
    uint8_t best_phase = 0u;
    uint8_t cut_index = 0u;
    auto index = ntk.node_to_index( n );

    auto& node_data = node_match[index];
    auto& cut_matches = matches[index];
    supergate<NInputs> const* best_supergate = node_data.best_supergate[phase];

    /* recompute best match info */
    if ( best_supergate != nullptr )
    {
      auto const& cut = cuts.cuts( index )[node_data.best_cut[phase]];

      best_phase = node_data.phase[phase];
      best_arrival = 0.0f;
      best_area_flow = best_supergate->area + cut_leaves_flow( cut, n, phase );
      best_area = best_supergate->area;
      best_cut = node_data.best_cut[phase];
      best_size = cut.size();

      auto ctr = 0u;
      for ( auto l : cut )
      {
        double arrival_pin = node_match[l].arrival[( best_phase >> ctr ) & 1] + best_supergate->tdelay[ctr];
        best_arrival = std::max( best_arrival, arrival_pin );
        ++ctr;
      }
    }

    /* foreach cut */
    for ( auto& cut : cuts.cuts( index ) )
    {
      /* trivial cuts or not matched cuts */
      if ( ( *cut )->data.ignore )
      {
        ++cut_index;
        continue;
      }

      auto const& supergates = cut_matches[( *cut )->data.match_index].supergates;
      auto const negation = cut_matches[( *cut )->data.match_index].negations[phase];

      if ( supergates[phase] == nullptr )
      {
        ++cut_index;
        continue;
      }

      /* match each gate and take the best one */
      for ( auto const& gate : *supergates[phase] )
      {
        uint8_t gate_polarity = gate.polarity ^ negation;
        node_data.phase[phase] = gate_polarity;
        double area_local = gate.area + cut_leaves_flow( *cut, n, phase );
        double worst_arrival = 0.0f;

        auto ctr = 0u;
        for ( auto l : *cut )
        {
          double arrival_pin = node_match[l].arrival[( gate_polarity >> ctr ) & 1] + gate.tdelay[ctr];
          worst_arrival = std::max( worst_arrival, arrival_pin );
          ++ctr;
        }

        if constexpr ( DO_AREA )
        {
          if ( worst_arrival > node_data.required[phase] + epsilon )
            continue;
        }
        
        if ( compare_map<DO_AREA>( worst_arrival, best_arrival, area_local, best_area_flow, cut->size(), best_size ) )
        {
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

  template<bool SwitchActivity>
  void match_phase_exact( node<Ntk> const& n, uint8_t phase )
  {
    double best_arrival = std::numeric_limits<double>::max();
    float best_exact_area = std::numeric_limits<float>::max();
    float best_area = std::numeric_limits<float>::max();
    uint32_t best_size = UINT32_MAX;
    uint8_t best_cut = 0u;
    uint8_t best_phase = 0u;
    uint8_t cut_index = 0u;
    auto index = ntk.node_to_index( n );

    auto& node_data = node_match[index];
    auto& cut_matches = matches[index];
    supergate<NInputs> const* best_supergate = node_data.best_supergate[phase];

    /* recompute best match info */
    if ( best_supergate != nullptr )
    {
      auto const& cut = cuts.cuts( index )[node_data.best_cut[phase]];

      best_phase = node_data.phase[phase];
      best_arrival = 0.0f;
      best_area = best_supergate->area;
      best_cut = node_data.best_cut[phase];
      best_size = cut.size();

      auto ctr = 0u;
      for ( auto l : cut )
      {
        double arrival_pin = node_match[l].arrival[( best_phase >> ctr ) & 1] + best_supergate->tdelay[ctr];
        best_arrival = std::max( best_arrival, arrival_pin );
        ++ctr;
      }

      /* if cut is implemented, remove it from the cover */
      if ( !node_data.same_match && node_data.map_refs[phase] )
      {
        best_exact_area = cut_deref<SwitchActivity>( cuts.cuts( index )[best_cut], n, phase );
      }
      else
      {
        best_exact_area = cut_ref<SwitchActivity>( cuts.cuts( index )[best_cut], n, phase );
        cut_deref<SwitchActivity>( cuts.cuts( index )[best_cut], n, phase );
      }
    }

    /* foreach cut */
    for ( auto& cut : cuts.cuts( index ) )
    {
      /* trivial cuts or not matched cuts */
      if ( ( *cut )->data.ignore )
      {
        ++cut_index;
        continue;
      }

      auto const& supergates = cut_matches[( *cut )->data.match_index].supergates;
      auto const negation = cut_matches[( *cut )->data.match_index].negations[phase];

      if ( supergates[phase] == nullptr )
      {
        ++cut_index;
        continue;
      }

      /* match each gate and take the best one */
      for ( auto const& gate : *supergates[phase] )
      {
        uint8_t gate_polarity = gate.polarity ^ negation;
        node_data.phase[phase] = gate_polarity;
        node_data.area[phase] = gate.area;
        float area_exact = cut_ref<SwitchActivity>( *cut, n, phase );
        cut_deref<SwitchActivity>( *cut, n, phase );
        double worst_arrival = 0.0f;

        auto ctr = 0u;
        for ( auto l : *cut )
        {
          double arrival_pin = node_match[l].arrival[( gate_polarity >> ctr ) & 1] + gate.tdelay[ctr];
          worst_arrival = std::max( worst_arrival, arrival_pin );
          ++ctr;
        }

        if ( worst_arrival > node_data.required[phase] + epsilon )
          continue;

        if ( compare_map<true>( worst_arrival, best_arrival, area_exact, best_exact_area, cut->size(), best_size ) )
        {
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

    if ( !node_data.same_match && node_data.map_refs[phase] )
    {
      best_exact_area = cut_ref<SwitchActivity>( cuts.cuts( index )[best_cut], n, phase );
    }
  }

  node_position compute_gate_position(cut_t const& cut) 
  {
    node_position gate_position {0, 0};
    int crt = 0;
    for (auto& c : cut.pins) {
      gate_position.x_coordinate += np[c].x_coordinate;
      gate_position.y_coordinate += np[c].y_coordinate;
      crt++;
    }
    gate_position.x_coordinate = gate_position.x_coordinate / crt;
    gate_position.y_coordinate = gate_position.y_coordinate / crt;
    return gate_position;
  }

  template <bool DO_TOTALWIRE, bool DO_POWER, bool DO_TRADE>
  void match_wirelength(node<Ntk> const& n, uint8_t phase) {
    double best_arrival = std::numeric_limits<double>::max();
    double best_area_flow = std::numeric_limits<double>::max();
    float best_area = std::numeric_limits<float>::max();
    double best_wirelength = std::numeric_limits<double>::max();
    double best_total_wirelength = std::numeric_limits<double>::max();
    bool best_position = false;
    uint32_t best_size = UINT32_MAX;
    uint8_t best_cut = 0u;
    uint8_t best_phase = 0u;
    uint8_t cut_index = 0u;
    auto index = ntk.node_to_index(n);

    auto& node_data = node_match[index];
    auto& cut_matches = matches[index];
    supergate<NInputs> const* best_supergate = node_data.best_supergate[phase];
    auto const& cur_best_cut = cuts.cuts(index)[node_data.best_cut[phase]];

    node_position best_gate_position = compute_gate_position(cur_best_cut);

    std::cout << "1position of node / phase: " << n << " / " << int(phase) << " = " 
              << best_gate_position.x_coordinate << " " 
              << best_gate_position.y_coordinate << std::endl; 

    /* recompute best match info */
    if (best_supergate != nullptr) {
      std::cout << "best_supergate is not nullptr" << std::endl;
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

      if (cut.size() != 1) {
        best_gate_position = compute_gate_position(cut);
      } else {
        std::cerr << "Error: This cut has only one leaf" << std::endl;
        best_gate_position = np[index];
      }
      best_wirelength =
          compute_match_wirelength(cut, best_gate_position, best_phase);
      best_total_wirelength =
          compute_match_total_wirelength(cut, best_gate_position, best_phase);
    }

    /* foreach cut */
    for (auto& cut : cuts.cuts(index)) {
      std::cout << "cut_index: " << static_cast<int>(cut_index) << std::endl;
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
        std::cerr << "Error: This cut has only one leaf" << std::endl;
        gate_position = np[index];
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

        if (worst_arrival > node_data.required[phase] + epsilon) {
          std::cout << "arrival > required" << std::endl;
          continue;
        }

        // if (!DO_TRADE) {
        //   if constexpr (!DO_POWER) {
        //     if constexpr (DO_TOTALWIRE) {
        //       if (worst_wirelength > node_data.wirelength[phase] + epsilon)
        //         continue;
        //     }
        //   }
        // }

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
          best_gate_position = gate_position;
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
    node_data.position[phase] = best_gate_position;
    std::cout << "2position of node / phase: " << n << " / " << int(phase) << " = " 
              << node_data.position[phase].x_coordinate << " " 
              << node_data.position[phase].y_coordinate << std::endl; 
    if (best_position) {
      std::cout << "replace success, position of node / phase: " << std::endl;
    }
  }

  void match_wirelength_exact(node<Ntk> const& n, uint8_t phase) {
    double best_arrival = std::numeric_limits<double>::max();
    float best_exact_area = std::numeric_limits<float>::max();
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

      if (cut.size() != 1)
        best_gate_position = compute_gate_position(cut);
      else {
        std::cerr << "Error: This cut has only one leaf" << std::endl;
        best_gate_position = np[index];
      }
      best_wirelength =
          compute_match_wirelength(cut, best_gate_position, best_phase);
      best_total_wirelength =
          compute_match_total_wirelength(cut, best_gate_position, best_phase);

      /* if cut is implemented, remove it from the cover */
      if (!node_data.same_match && node_data.map_refs[phase]) {
        best_exact_area =
            cut_deref<false>(cuts.cuts(index)[best_cut], n, phase);
      } else {
        best_exact_area = cut_ref<false>(cuts.cuts(index)[best_cut], n, phase);
        cut_deref<false>(cuts.cuts(index)[best_cut], n, phase);
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

      node_position gate_position;
      if ((*cut).size() != 1)
        gate_position = compute_gate_position(*cut);
      else {
        std::cerr << "Error: This cut has only one leaf" << std::endl;
        gate_position = np[index];
      }

      /* match each gate and take the best one */
      for (auto const& gate : *supergates[phase]) {
        uint8_t gate_polarity = gate.polarity ^ negation;
        node_data.phase[phase] = gate_polarity;
        node_data.area[phase] = gate.area;
        float area_exact = cut_ref<false>(*cut, n, phase);
        cut_deref<false>(*cut, n, phase);
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
        if (worst_wirelength * 0.9 > node_data.wirelength[phase] + epsilon)
          continue;
        if (worst_total_wirelength * 0.9 >
            node_data.total_wirelength[phase] + epsilon)
          continue;

        if (compare_map<true>(worst_arrival, best_arrival, area_exact,
                              best_exact_area, cut->size(), best_size)) {
          best_wirelength = worst_wirelength;
          best_total_wirelength = worst_total_wirelength;
          best_arrival = worst_arrival;
          best_exact_area = area_exact;
          best_area = gate.area;
          best_size = cut->size();
          best_cut = cut_index;
          best_phase = gate_polarity;
          best_supergate = &gate;
          best_gate_position = gate_position;
          best_position = true;
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
    node_data.wirelength[phase] = best_wirelength;
    node_data.total_wirelength[phase] = best_total_wirelength;
    node_data.position[phase] = best_gate_position;

    if (!node_data.same_match && node_data.map_refs[phase]) {
      best_exact_area =
          cut_ref<false>(cuts.cuts(index)[best_cut], n, phase);
    }
  }

  template<bool DO_AREA, bool ELA>
  void match_drop_phase( node<Ntk> const& n, float required_margin_factor )
  {
    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];

    /* compute arrival adding an inverter to the other match phase */
    double worst_arrival_npos = node_data.arrival[1] + lib_inv_delay;
    double worst_arrival_nneg = node_data.arrival[0] + lib_inv_delay;
    bool use_zero = false;
    bool use_one = false;

    /* only one phase is matched */
    if ( node_data.best_supergate[0] == nullptr )
    {
      set_match_complemented_phase( index, 1, worst_arrival_npos );
      if constexpr ( ELA )
      {
        if ( node_data.map_refs[2] )
          cut_ref<false>( cuts.cuts( index )[node_data.best_cut[1]], n, 1 );
      }
      return;
    }
    else if ( node_data.best_supergate[1] == nullptr )
    {
      set_match_complemented_phase( index, 0, worst_arrival_nneg );
      if constexpr ( ELA )
      {
        if ( node_data.map_refs[2] )
          cut_ref<false>( cuts.cuts( index )[node_data.best_cut[0]], n, 0 );
      }
      return;
    }

    /* try to use only one match to cover both phases */
    if constexpr ( !DO_AREA )
    {
      /* if arrival improves matching the other phase and inserting an inverter */
      if ( worst_arrival_npos < node_data.arrival[0] + epsilon )
      {
        use_one = true;
      }
      if ( worst_arrival_nneg < node_data.arrival[1] + epsilon )
      {
        use_zero = true;
      }
    }
    else
    {
      /* check if both phases + inverter meet the required time */
      use_zero = worst_arrival_nneg < ( node_data.required[1] + epsilon - required_margin_factor * lib_inv_delay );
      use_one = worst_arrival_npos < ( node_data.required[0] + epsilon - required_margin_factor * lib_inv_delay );
    }

    /* condition on not used phases, evaluate a substitution during exact area recovery */
    if constexpr ( ELA )
    {
      if ( iteration != 0 )
      {
        if ( node_data.map_refs[0] == 0 || node_data.map_refs[1] == 0 )
        {
          /* select the used match */
          auto phase = 0;
          auto nphase = 0;
          if ( node_data.map_refs[0] == 0 )
          {
            phase = 1;
            use_one = true;
            use_zero = false;
          }
          else
          {
            nphase = 1;
            use_one = false;
            use_zero = true;
          }
          /* select the not used match instead if it leads to area improvement and doesn't violate the required time */
          if ( node_data.arrival[nphase] + lib_inv_delay < node_data.required[phase] + epsilon )
          {
            auto size_phase = cuts.cuts( index )[node_data.best_cut[phase]].size();
            auto size_nphase = cuts.cuts( index )[node_data.best_cut[nphase]].size();

            if ( compare_map<DO_AREA>( node_data.arrival[nphase] + lib_inv_delay, node_data.arrival[phase], node_data.flows[nphase] + lib_inv_area, node_data.flows[phase], size_nphase, size_phase ) )
            {
              /* invert the choice */
              use_zero = !use_zero;
              use_one = !use_one;
            }
          }
        }
      }
    }

    if ( !use_zero && !use_one )
    {
      /* use both phases */
      node_data.flows[0] = node_data.flows[0] / node_data.est_refs[0];
      node_data.flows[1] = node_data.flows[1] / node_data.est_refs[1];
      node_data.flows[2] = node_data.flows[0] + node_data.flows[1];
      node_data.same_match = false;
      return;
    }

    /* use area flow as a tiebreaker */
    if ( use_zero && use_one )
    {
      auto size_zero = cuts.cuts( index )[node_data.best_cut[0]].size();
      auto size_one = cuts.cuts( index )[node_data.best_cut[1]].size();
      if ( compare_map<DO_AREA>( worst_arrival_nneg, worst_arrival_npos, node_data.flows[0], node_data.flows[1], size_zero, size_one ) )
        use_one = false;
      else
        use_zero = false;
    }

    if ( use_zero )
    {
      if constexpr ( ELA )
      {
        /* set cut references */
        if ( !node_data.same_match )
        {
          /* dereference the negative phase cut if in use */
          if ( node_data.map_refs[1] > 0 )
            cut_deref<false>( cuts.cuts( index )[node_data.best_cut[1]], n, 1 );
          /* reference the positive cut if not in use before */
          if ( node_data.map_refs[0] == 0 && node_data.map_refs[2] )
            cut_ref<false>( cuts.cuts( index )[node_data.best_cut[0]], n, 0 );
        }
        else if ( node_data.map_refs[2] )
          cut_ref<false>( cuts.cuts( index )[node_data.best_cut[0]], n, 0 );
      }
      set_match_complemented_phase( index, 0, worst_arrival_nneg );
    }
    else
    {
      if constexpr ( ELA )
      {
        /* set cut references */
        if ( !node_data.same_match )
        {
          /* dereference the positive phase cut if in use */
          if ( node_data.map_refs[0] > 0 )
            cut_deref<false>( cuts.cuts( index )[node_data.best_cut[0]], n, 0 );
          /* reference the negative cut if not in use before */
          if ( node_data.map_refs[1] == 0 && node_data.map_refs[2] )
            cut_ref<false>( cuts.cuts( index )[node_data.best_cut[1]], n, 1 );
        }
        else if ( node_data.map_refs[2] )
          cut_ref<false>( cuts.cuts( index )[node_data.best_cut[1]], n, 1 );
      }
      set_match_complemented_phase( index, 1, worst_arrival_npos );
    }
  }

  template <bool DO_TOTALWIRE, bool DO_TRADE>
  void match_wirelength_drop_phase(node<Ntk> const& n) {
    auto index = ntk.node_to_index(n);
    auto& node_data = node_match[index];

    /* compute arrival adding an inverter to the other match phase */
    double worst_arrival_npos = node_data.arrival[1] + lib_inv_delay;
    double worst_arrival_nneg = node_data.arrival[0] + lib_inv_delay;
    double worst_wirelength_npos = node_data.wirelength[1];
    double worst_wirelength_nneg = node_data.wirelength[0];
    double worst_total_wirelength_npos = node_data.total_wirelength[1];
    double worst_total_wirelength_nneg = node_data.total_wirelength[0];
    bool use_zero = false;
    bool use_one = false;

    /* only one phase is matched */
    if (node_data.best_supergate[0] == nullptr) {
      set_match_wirelength_complemented_phase<DO_TOTALWIRE>(index, 1, worst_arrival_npos);
      return;
    } else if (node_data.best_supergate[1] == nullptr) {
      set_match_wirelength_complemented_phase<DO_TOTALWIRE>(index, 0, worst_arrival_nneg);
      return;
    }

    /* try to use only one match to cover both phases */
    /* if arrival improves matching the other phase and inserting an inverter
     */
    if (worst_arrival_npos < node_data.arrival[0] + epsilon) {
      use_one = true;
    }
    if (worst_arrival_nneg < node_data.arrival[1] + epsilon) {
      use_zero = true;
    }

    if (!use_zero && !use_one) {
      /* use both phases */
      node_data.flows[0] = node_data.flows[0] / node_data.est_refs[0];
      node_data.flows[1] = node_data.flows[1] / node_data.est_refs[1];
      node_data.flows[2] = node_data.flows[0] + node_data.flows[1];

      // Treat the congest flow as the area flow
      node_data.congest_flow[0] = node_data.congest_flow[0] / node_data.est_refs[0];
      node_data.congest_flow[1] = node_data.congest_flow[1] / node_data.est_refs[1];
      node_data.congest_flow[2] = node_data.congest_flow[0] + node_data.congest_flow[1];

      // Treat the total wirelength as the area flow
      if constexpr (DO_TOTALWIRE) {
        node_data.total_wirelength[0] = node_data.total_wirelength[0] / node_data.est_refs[0];
        node_data.total_wirelength[1] = node_data.total_wirelength[1] / node_data.est_refs[1];
        node_data.total_wirelength[2] = node_data.total_wirelength[0] + node_data.total_wirelength[1];
      }

      node_data.same_match = false;
      return;
    }

    /* use area flow as a tiebreaker */
    if (use_zero && use_one) {
      auto size_zero = cuts.cuts(index)[node_data.best_cut[0]].size();
      auto size_one = cuts.cuts(index)[node_data.best_cut[1]].size();
      if (compare_map_wirelength<DO_TOTALWIRE, DO_TRADE>(
              worst_wirelength_nneg, worst_wirelength_npos, worst_arrival_nneg,
              worst_arrival_npos, worst_total_wirelength_nneg,
              worst_total_wirelength_npos, size_zero, size_one))
        use_one = false;
      else
        use_zero = false;
    }

    if (use_zero) {
      set_match_wirelength_complemented_phase<DO_TOTALWIRE>(index, 0, worst_arrival_nneg);
    } else {
      set_match_wirelength_complemented_phase<DO_TOTALWIRE>(index, 1, worst_arrival_npos);
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

  template <bool DO_TOTALWIRE>
  inline void set_match_wirelength_complemented_phase(uint32_t index, uint8_t phase,
                                                      double worst_arrival_n) {
    auto& node_data = node_match[index];
    auto phase_n = phase ^ 1;
    node_data.same_match = true;
    node_data.best_supergate[phase_n] = nullptr;
    node_data.best_cut[phase_n] = node_data.best_cut[phase];
    node_data.phase[phase_n] = node_data.phase[phase];
    node_data.arrival[phase_n] = worst_arrival_n;
    node_data.wirelength[phase_n] = node_data.wirelength[phase];
    node_data.position[phase_n] = node_data.position[phase];

    // Treat the total wirelength as the area flow
    if constexpr (DO_TOTALWIRE) {
      node_data.total_wirelength[phase] = node_data.total_wirelength[phase] / node_data.est_refs[2];
      node_data.total_wirelength[phase_n] = node_data.total_wirelength[phase];
    }

    node_data.area[phase_n] = node_data.area[phase];
    node_data.congestion[phase_n] = node_data.congestion[phase];
    node_data.position[phase_n] = node_data.position[phase];

    node_data.flows[phase] = node_data.flows[phase] / node_data.est_refs[2];
    node_data.flows[phase_n] = node_data.flows[phase];

    // Treat the congest flow as the area flow
    node_data.congest_flow[phase] = node_data.congest_flow[phase] / node_data.est_refs[2];
    node_data.congest_flow[phase_n] = node_data.congest_flow[phase];

    node_data.flows[2] = node_data.flows[phase];
    node_data.congest_flow[2] = node_data.congest_flow[phase];
    node_data.total_wirelength[2] = node_data.total_wirelength[phase];
  }

  void match_constants( uint32_t index )
  {
    auto& node_data = node_match[index];

    kitty::static_truth_table<6> zero_tt;
    auto const supergates_zero = library.get_supergates( zero_tt );
    auto const supergates_one = library.get_supergates( ~zero_tt );

    /* Not available in the library */
    if ( supergates_zero == nullptr && supergates_one == nullptr )
    {
      return;
    }
    /* if only one is available, the other is obtained using an inverter */
    if ( supergates_zero != nullptr )
    {
      node_data.best_supergate[0] = &( ( *supergates_zero )[0] );
      node_data.arrival[0] = node_data.best_supergate[0]->tdelay[0];
      node_data.area[0] = node_data.best_supergate[0]->area;
      node_data.phase[0] = 0;
    }
    if ( supergates_one != nullptr )
    {
      node_data.best_supergate[1] = &( ( *supergates_one )[0] );
      node_data.arrival[1] = node_data.best_supergate[1]->tdelay[0];
      node_data.area[1] = node_data.best_supergate[1]->area;
      node_data.phase[1] = 0;
    }
    else
    {
      node_data.same_match = true;
      node_data.arrival[1] = node_data.arrival[0] + lib_inv_delay;
      node_data.area[1] = node_data.area[0] + lib_inv_area;
      node_data.phase[1] = 1;
    }
    if ( supergates_zero == nullptr )
    {
      node_data.same_match = true;
      node_data.arrival[0] = node_data.arrival[1] + lib_inv_delay;
      node_data.area[0] = node_data.area[1] + lib_inv_area;
      node_data.phase[0] = 1;
    }
  }

  inline double cut_leaves_flow( cut_t const& cut, node<Ntk> const& n, uint8_t phase )
  {
    double flow{ 0.0f };
    auto const& node_data = node_match[ntk.node_to_index( n )];

    uint8_t ctr = 0u;
    for ( auto leaf : cut )
    {
      uint8_t leaf_phase = ( node_data.phase[phase] >> ctr++ ) & 1;
      flow += node_match[leaf].flows[leaf_phase];
    }

    return flow;
  }

  template<bool SwitchActivity>
  float cut_ref( cut_t const& cut, node<Ntk> const& n, uint8_t phase )
  {
    auto const& node_data = node_match[ntk.node_to_index( n )];
    float count;

    if constexpr ( SwitchActivity )
      count = switch_activity[ntk.node_to_index( n )];
    else
      count = node_data.area[phase];

    uint8_t ctr = 0;
    for ( auto leaf : cut )
    {
      /* compute leaf phase using the current gate */
      uint8_t leaf_phase = ( node_data.phase[phase] >> ctr++ ) & 1;

      if ( ntk.is_constant( ntk.index_to_node( leaf ) ) )
      {
        continue;
      }
      else if ( ntk.is_ci( ntk.index_to_node( leaf ) ) )
      {
        /* reference PIs, add inverter cost for negative phase */
        if ( leaf_phase == 1u )
        {
          if ( node_match[leaf].map_refs[1]++ == 0u )
          {
            if constexpr ( SwitchActivity )
              count += switch_activity[leaf];
            else
              count += lib_inv_area;
          }
        }
        else
        {
          ++node_match[leaf].map_refs[0];
        }
        continue;
      }

      if ( node_match[leaf].same_match )
      {
        /* Add inverter area if not present yet and leaf node is implemented in the opposite phase */
        if ( node_match[leaf].map_refs[leaf_phase]++ == 0u && node_match[leaf].best_supergate[leaf_phase] == nullptr )
        {
          if constexpr ( SwitchActivity )
            count += switch_activity[leaf];
          else
            count += lib_inv_area;
        }
        /* Recursive referencing if leaf was not referenced */
        if ( node_match[leaf].map_refs[2]++ == 0u )
        {
          count += cut_ref<SwitchActivity>( cuts.cuts( leaf )[node_match[leaf].best_cut[leaf_phase]], ntk.index_to_node( leaf ), leaf_phase );
        }
      }
      else
      {
        ++node_match[leaf].map_refs[2];
        if ( node_match[leaf].map_refs[leaf_phase]++ == 0u )
        {
          count += cut_ref<SwitchActivity>( cuts.cuts( leaf )[node_match[leaf].best_cut[leaf_phase]], ntk.index_to_node( leaf ), leaf_phase );
        }
      }
    }
    return count;
  }

  template<bool SwitchActivity>
  float cut_deref( cut_t const& cut, node<Ntk> const& n, uint8_t phase )
  {
    auto const& node_data = node_match[ntk.node_to_index( n )];
    float count;

    if constexpr ( SwitchActivity )
      count = switch_activity[ntk.node_to_index( n )];
    else
      count = node_data.area[phase];

    uint8_t ctr = 0;
    for ( auto leaf : cut )
    {
      /* compute leaf phase using the current gate */
      uint8_t leaf_phase = ( node_data.phase[phase] >> ctr++ ) & 1;

      if ( ntk.is_constant( ntk.index_to_node( leaf ) ) )
      {
        continue;
      }
      else if ( ntk.is_ci( ntk.index_to_node( leaf ) ) )
      {
        /* dereference PIs, add inverter cost for negative phase */
        if ( leaf_phase == 1u )
        {
          if ( --node_match[leaf].map_refs[1] == 0u )
          {
            if constexpr ( SwitchActivity )
              count += switch_activity[leaf];
            else
              count += lib_inv_area;
          }
        }
        else
        {
          --node_match[leaf].map_refs[0];
        }
        continue;
      }

      if ( node_match[leaf].same_match )
      {
        /* Add inverter area if it is used only by the current gate and leaf node is implemented in the opposite phase */
        if ( --node_match[leaf].map_refs[leaf_phase] == 0u && node_match[leaf].best_supergate[leaf_phase] == nullptr )
        {
          if constexpr ( SwitchActivity )
            count += switch_activity[leaf];
          else
            count += lib_inv_area;
        }
        /* Recursive dereferencing */
        if ( --node_match[leaf].map_refs[2] == 0u )
        {
          count += cut_deref<SwitchActivity>( cuts.cuts( leaf )[node_match[leaf].best_cut[leaf_phase]], ntk.index_to_node( leaf ), leaf_phase );
        }
      }
      else
      {
        --node_match[leaf].map_refs[2];
        if ( --node_match[leaf].map_refs[leaf_phase] == 0u )
        {
          count += cut_deref<SwitchActivity>( cuts.cuts( leaf )[node_match[leaf].best_cut[leaf_phase]], ntk.index_to_node( leaf ), leaf_phase );
        }
      }
    }
    return count;
  }

  void insert_buffers()
  {
    if ( lib_buf_id != UINT32_MAX )
    {
      double area_old = area;
      bool buffers = false;

      ntk.foreach_co( [&]( auto const& f ) {
        auto const& n = ntk.get_node( f );
        if ( !ntk.is_constant( n ) && ntk.is_ci( n ) && !ntk.is_complemented( f ) )
        {
          area += lib_buf_area;
          delay = std::max( delay, node_match[ntk.node_to_index( n )].arrival[0] + lib_inv_delay );
          buffers = true;
        }
      } );

      /* round stats */
      if ( ps.verbose && buffers )
      {
        std::stringstream stats{};
        float area_gain = 0.0f;

        area_gain = float( ( area_old - area ) / area_old * 100 );

        stats << fmt::format( "[i] Buffering: Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n", delay, area, area_gain );
        st.round_stats.push_back( stats.str() );
      }
    }
  }

  template<bool PHYAWARE = false>
  std::pair<map_ntk_t, klut_map> initialize_map_network()
  {
    map_ntk_t dest( library.get_gates() );
    klut_map old2new;

    if constexpr ( PHYAWARE ) {
      match_position.clear();
      match_position.resize(2);
    }
      

    std::cout << "initially dest size = " << dest.size() << std::endl;

    old2new[ntk.node_to_index( ntk.get_node( ntk.get_constant( false ) ) )][0] = dest.get_constant( false );
    old2new[ntk.node_to_index( ntk.get_node( ntk.get_constant( false ) ) )][1] = dest.get_constant( true );

    ntk.foreach_pi( [&]( auto const& n ) {
      old2new[ntk.node_to_index( n )][0] = dest.create_pi();
      if constexpr ( PHYAWARE ) {
        match_position.push_back( node_match[ntk.node_to_index( n )].position[0] );
        std::cout << "match_position [" << (match_position.size() - 1) << "] = " 
                      << match_position[match_position.size() - 1].x_coordinate << ", " 
                      << match_position[match_position.size() - 1].y_coordinate << std::endl;
        if ( match_position.size() != dest.size() )
          std::cerr << "Error: match_position size is not equal to dest size: index = " << ntk.node_to_index( n )
                    << " match size = " << match_position.size() << " dest size = " << dest.size() << std::endl;
      }
    } );

    return { dest, old2new };
  }

  std::pair<seq_map_ntk_t, klut_map> initialize_map_seq_network()
  {
    seq_map_ntk_t dest( library.get_gates() );
    klut_map old2new;

    old2new[ntk.node_to_index( ntk.get_node( ntk.get_constant( false ) ) )][0] = dest.get_constant( false );
    old2new[ntk.node_to_index( ntk.get_node( ntk.get_constant( false ) ) )][1] = dest.get_constant( true );

    ntk.foreach_pi( [&]( auto const& n ) {
      old2new[ntk.node_to_index( n )][0] = dest.create_pi();
    } );
    ntk.foreach_ro( [&]( auto const& n ) {
      old2new[ntk.node_to_index( n )][0] = dest.create_ro();
    } );

    return { dest, old2new };
  }

  std::vector<signal<klut_network>>& get_binding_roots(node<Ntk> const& n) {
    auto index = ntk.node_to_index(n);
    return _binding_roots[index];
  }

  template<class NtkDest, bool PHYAWARE>
  void finalize_cover_congest( NtkDest& res, klut_map& old2new)
  {
    for ( auto const& n : top_order )
    {
      auto index = ntk.node_to_index( n );
      auto const& node_data = node_match[index];

      /* add inverter at PI if needed */
      if ( ntk.is_constant( n ) )
      {
        if ( node_data.best_supergate[0] == nullptr && node_data.best_supergate[1] == nullptr )
          continue;
      }
      else if ( ntk.is_ci( n ) )
      {
        if ( node_data.map_refs[1] > 0 )
        {
          old2new[index][1] = res.create_not( old2new[n][0] );
          if constexpr ( PHYAWARE ) {
            match_position.push_back(node_data.position[0]);
            std::cout << "match_position [" << (match_position.size() - 1) << "] = " 
                      << match_position[match_position.size() - 1].x_coordinate << ", " 
                      << match_position[match_position.size() - 1].y_coordinate << std::endl;
            if (match_position.size() != res.size())
              std::cerr << "Error: match_position size is not equal to res size: index = " << index
                 << " match size = " << match_position.size() << " res size = " << res.size() << std::endl;
          }
          
          res.add_binding( res.get_node( old2new[index][1] ), lib_inv_id );
        }
        continue;
      }

      /* continue if cut is not in the cover */
      if ( node_data.map_refs[2] == 0u )
        continue;

      unsigned phase = ( node_data.best_supergate[0] != nullptr ) ? 0 : 1;

      /* add used cut */
      if ( node_data.same_match || node_data.map_refs[phase] > 0 )
      {
        create_lut_for_gate<NtkDest>( res, old2new, index, phase );
        if constexpr ( PHYAWARE ) {
          auto& best_cut = cuts.cuts( index )[node_data.best_cut[phase]];
          auto& clustered_nodes = best_cut.nodes;
          for (auto const& node : clustered_nodes) {
            auto& binding_roots = get_binding_roots(node);
            binding_roots.push_back(old2new[index][phase]);
          }
          match_position.push_back(node_data.position[phase]);
          std::cout << "match_position [" << (match_position.size() - 1) << "] = " 
                      << match_position[match_position.size() - 1].x_coordinate << ", " 
                      << match_position[match_position.size() - 1].y_coordinate << std::endl;
          if (match_position.size() != res.size())
              std::cerr << "Error: match_position size is not equal to res size: index = " << index
                 << " match size = " << match_position.size() << " res size = " << res.size() << std::endl;
        }

        /* add inverted version if used */
        if ( node_data.same_match && node_data.map_refs[phase ^ 1] > 0 )
        {
          old2new[index][phase ^ 1] = res.create_not( old2new[index][phase] );
          if constexpr (PHYAWARE) {
            match_position.push_back(node_data.position[phase ^ 1]);
            std::cout << "match_position [" << (match_position.size() - 1) << "] = " 
                      << match_position[match_position.size() - 1].x_coordinate << ", " 
                      << match_position[match_position.size() - 1].y_coordinate << std::endl;
            if (match_position.size() != res.size())
              std::cerr << "Error: match_position size is not equal to res size: index = " << index
                 << " match size = " << match_position.size() << " res size = " << res.size() << std::endl;
          }
          res.add_binding( res.get_node( old2new[index][phase ^ 1] ), lib_inv_id );
        }
      }

      phase = phase ^ 1;
      /* add the optional other match if used */
      if ( !node_data.same_match && node_data.map_refs[phase] > 0 )
      {
        create_lut_for_gate<NtkDest>( res, old2new, index, phase );
        if constexpr (PHYAWARE) {
          auto& best_cut = cuts.cuts(index)[node_data.best_cut[phase]];
          auto& clustered_nodes = best_cut.nodes;
          for (auto const& node : clustered_nodes) {
            auto& binding_roots = get_binding_roots(node);
            binding_roots.push_back(old2new[index][phase]);
          }
          match_position.push_back(node_data.position[phase]);
          std::cout << "match_position [" << (match_position.size() - 1) << "] = " 
                      << match_position[match_position.size() - 1].x_coordinate << ", " 
                      << match_position[match_position.size() - 1].y_coordinate << std::endl;
          if (match_position.size() != res.size())
              std::cerr << "Error: match_position size is not equal to res size: index = " << index
                 << " match size = " << match_position.size() << " res size = " << res.size() << std::endl;
        }
      }
    }

    /* create POs */
    ntk.foreach_po( [&]( auto const& f ) {
      if ( ntk.is_complemented( f ) )
      {
        res.create_po( old2new[ntk.node_to_index( ntk.get_node( f ) )][1] );
      }
      else if ( !ntk.is_constant( ntk.get_node( f ) ) && ntk.is_ci( ntk.get_node( f ) ) && lib_buf_id != UINT32_MAX )
      {
        /* create buffers for POs */
        static uint64_t _buf = 0x2;
        kitty::dynamic_truth_table tt_buf( 1 );
        kitty::create_from_words( tt_buf, &_buf, &_buf + 1 );
        const auto buf = res.create_node( { old2new[ntk.node_to_index( ntk.get_node( f ) )][0] }, tt_buf );
        res.create_po( buf );
        res.add_binding( res.get_node( buf ), lib_buf_id );
      }
      else
      {
        res.create_po( old2new[ntk.node_to_index( ntk.get_node( f ) )][0] );
      }
    } );

    if constexpr ( has_foreach_ri_v<Ntk> )
    {
      ntk.foreach_ri( [&]( auto const& f ) {
        if ( ntk.is_complemented( f ) )
        {
          res.create_ri( old2new[ntk.node_to_index( ntk.get_node( f ) )][1] );
        }
        else if ( !ntk.is_constant( ntk.get_node( f ) ) && ntk.is_ci( ntk.get_node( f ) ) && lib_buf_id != UINT32_MAX )
        {
          /* create buffers for RIs */
          static uint64_t _buf = 0x2;
          kitty::dynamic_truth_table tt_buf( 1 );
          kitty::create_from_words( tt_buf, &_buf, &_buf + 1 );
          const auto buf = res.create_node( { old2new[ntk.node_to_index( ntk.get_node( f ) )][0] }, tt_buf );
          res.create_ri( buf );
          res.add_binding( res.get_node( buf ), lib_buf_id );
        }
        else
        {
          res.create_ri( old2new[ntk.node_to_index( ntk.get_node( f ) )][0] );
        }
      } );
    }

    /* write final results */
    st.area = area;
    st.delay = delay;
    st.wirelength = wirelength;
    st.total_wirelength = total_wirelength;
    if ( ps.eswp_rounds )
      st.power = compute_switching_power();
  }

  template<class NtkDest>
  void finalize_cover( NtkDest& res, klut_map& old2new )
  {
    for ( auto const& n : top_order )
    {
      auto index = ntk.node_to_index( n );
      auto const& node_data = node_match[index];

      /* add inverter at PI if needed */
      if ( ntk.is_constant( n ) )
      {
        if ( node_data.best_supergate[0] == nullptr && node_data.best_supergate[1] == nullptr )
          continue;
      }
      else if ( ntk.is_ci( n ) )
      {
        if ( node_data.map_refs[1] > 0 )
        {
          old2new[index][1] = res.create_not( old2new[n][0] );
          res.add_binding( res.get_node( old2new[index][1] ), lib_inv_id );
        }
        continue;
      }

      /* continue if cut is not in the cover */
      if ( node_data.map_refs[2] == 0u )
        continue;

      unsigned phase = ( node_data.best_supergate[0] != nullptr ) ? 0 : 1;

      /* add used cut */
      if ( node_data.same_match || node_data.map_refs[phase] > 0 )
      {
        create_lut_for_gate<NtkDest>( res, old2new, index, phase );
        
        /* add inverted version if used */
        if ( node_data.same_match && node_data.map_refs[phase ^ 1] > 0 )
        {
          old2new[index][phase ^ 1] = res.create_not( old2new[index][phase] );
          res.add_binding( res.get_node( old2new[index][phase ^ 1] ), lib_inv_id );
        }
      }

      phase = phase ^ 1;
      /* add the optional other match if used */
      if ( !node_data.same_match && node_data.map_refs[phase] > 0 )
      {
        create_lut_for_gate<NtkDest>( res, old2new, index, phase );
      }
    }

    /* create POs */
    ntk.foreach_po( [&]( auto const& f ) {
      if ( ntk.is_complemented( f ) )
      {
        res.create_po( old2new[ntk.node_to_index( ntk.get_node( f ) )][1] );
      }
      else if ( !ntk.is_constant( ntk.get_node( f ) ) && ntk.is_ci( ntk.get_node( f ) ) && lib_buf_id != UINT32_MAX )
      {
        /* create buffers for POs */
        static uint64_t _buf = 0x2;
        kitty::dynamic_truth_table tt_buf( 1 );
        kitty::create_from_words( tt_buf, &_buf, &_buf + 1 );
        const auto buf = res.create_node( { old2new[ntk.node_to_index( ntk.get_node( f ) )][0] }, tt_buf );
        res.create_po( buf );
        res.add_binding( res.get_node( buf ), lib_buf_id );
      }
      else
      {
        res.create_po( old2new[ntk.node_to_index( ntk.get_node( f ) )][0] );
      }
    } );

    if constexpr ( has_foreach_ri_v<Ntk> )
    {
      ntk.foreach_ri( [&]( auto const& f ) {
        if ( ntk.is_complemented( f ) )
        {
          res.create_ri( old2new[ntk.node_to_index( ntk.get_node( f ) )][1] );
        }
        else if ( !ntk.is_constant( ntk.get_node( f ) ) && ntk.is_ci( ntk.get_node( f ) ) && lib_buf_id != UINT32_MAX )
        {
          /* create buffers for RIs */
          static uint64_t _buf = 0x2;
          kitty::dynamic_truth_table tt_buf( 1 );
          kitty::create_from_words( tt_buf, &_buf, &_buf + 1 );
          const auto buf = res.create_node( { old2new[ntk.node_to_index( ntk.get_node( f ) )][0] }, tt_buf );
          res.create_ri( buf );
          res.add_binding( res.get_node( buf ), lib_buf_id );
        }
        else
        {
          res.create_ri( old2new[ntk.node_to_index( ntk.get_node( f ) )][0] );
        }
      } );
    }

    /* write final results */
    st.area = area;
    st.delay = delay;
    st.wirelength = wirelength;
    st.total_wirelength = total_wirelength;
    if ( ps.eswp_rounds )
      st.power = compute_switching_power();
  }

  template<class NtkDest>
  void create_lut_for_gate( NtkDest& res, klut_map& old2new, uint32_t index, unsigned phase )
  {
    auto const& node_data = node_match[index];
    auto& best_cut = cuts.cuts( index )[node_data.best_cut[phase]];
    auto const& gate = node_data.best_supergate[phase]->root;

    /* permutate and negate to obtain the matched gate truth table */
    std::vector<signal<klut_network>> children( gate->num_vars );

    auto ctr = 0u;
    for ( auto l : best_cut )
    {
      if ( ctr >= gate->num_vars )
        break;
      children[node_data.best_supergate[phase]->permutation[ctr]] = old2new[l][( node_data.phase[phase] >> ctr ) & 1];
      ++ctr;
    }

    if ( !gate->is_super )
    {
      /* create the node */
      auto f = res.create_node( children, gate->function );
      res.add_binding( res.get_node( f ), gate->root->id );

      /* add the node in the data structure */
      old2new[index][phase] = f;
    }
    else
    {
      /* supergate, create sub-gates */
      auto f = create_lut_for_gate_rec<NtkDest>( res, *gate, children );

      /* add the node in the data structure */
      old2new[index][phase] = f;
    }
  }

  template<class NtkDest>
  signal<klut_network> create_lut_for_gate_rec( NtkDest& res, composed_gate<NInputs> const& gate, std::vector<signal<klut_network>> const& children )
  {
    std::vector<signal<klut_network>> children_local( gate.fanin.size() );

    auto i = 0u;
    for ( auto const fanin : gate.fanin )
    {
      if ( fanin->root == nullptr )
      {
        /* terminal condition */
        children_local[i] = children[fanin->id];
      }
      else
      {
        children_local[i] = create_lut_for_gate_rec<NtkDest>( res, *fanin, children );
      }
      ++i;
    }

    auto f = res.create_node( children_local, gate.root->function );
    res.add_binding( res.get_node( f ), gate.root->id );
    return f;
  }

  template<bool DO_AREA>
  inline bool compare_map( double arrival, double best_arrival, double area_flow, double best_area_flow, uint32_t size, uint32_t best_size )
  {
    if constexpr ( DO_AREA )
    {
      if ( area_flow < best_area_flow - epsilon )
      {
        return true;
      }
      else if ( area_flow > best_area_flow + epsilon )
      {
        return false;
      }
      else if ( arrival < best_arrival - epsilon )
      {
        return true;
      }
      else if ( arrival > best_arrival + epsilon )
      {
        return false;
      }
    }
    else
    {
      if ( arrival < best_arrival - epsilon )
      {
        return true;
      }
      else if ( arrival > best_arrival + epsilon )
      {
        return false;
      }
      else if ( area_flow < best_area_flow - epsilon )
      {
        return true;
      }
      else if ( area_flow > best_area_flow + epsilon )
      {
        return false;
      }
    }
    if ( size < best_size )
    {
      return true;
    }
    return false;
  }

  template <bool DO_TOTALWIRE, bool DO_TRADE>
  inline bool compare_map_wirelength(double wirelength, double best_wirelength,
                                     double arrival, double best_arrival,
                                     double total_wirelength,
                                     double best_total_wirelength,
                                     uint32_t size, uint32_t best_size) {
    if constexpr (DO_TRADE) {
      if (weight_w_d(wirelength / best_wirelength,
                     total_wirelength / best_total_wirelength,
                     arrival / best_arrival) < (1 - epsilon)) {
        return true;
      } else if (weight_w_d(wirelength / best_wirelength,
                            total_wirelength / best_total_wirelength,
                            arrival / best_arrival) > (1 + epsilon)) {
        return false;
      }
    } else {
      if constexpr (DO_TOTALWIRE) {
        if (total_wirelength < best_total_wirelength - epsilon)
          return true;
        else if (total_wirelength > best_total_wirelength + epsilon)
          return false;
        else if (wirelength < best_wirelength - epsilon)
          return true;
        else if (wirelength > best_wirelength + epsilon)
          return false;
        else if (arrival < best_arrival - epsilon)
          return true;
        else if (arrival > best_arrival + epsilon)
          return false;
      } else {
        if (wirelength < best_wirelength - epsilon)
          return true;
        else if (wirelength > best_wirelength + epsilon)
          return false;
        else if (arrival < best_arrival - epsilon)
          return true;
        else if (arrival > best_arrival + epsilon)
          return false;
        else if (total_wirelength < best_total_wirelength - epsilon)
          return true;
        else if (total_wirelength > best_total_wirelength + epsilon)
          return false;
      }
    }
    if (size < best_size) return true;
    return false;
  }

  double compute_switching_power()
  {
    double power = 0.0f;

    for ( auto const& n : top_order )
    {
      const auto index = ntk.node_to_index( n );
      auto& node_data = node_match[index];

      if ( ntk.is_constant( n ) )
      {
        if ( node_data.best_supergate[0] == nullptr && node_data.best_supergate[1] == nullptr )
          continue;
      }
      else if ( ntk.is_ci( n ) )
      {
        if ( node_data.map_refs[1] > 0 )
          power += switch_activity[ntk.node_to_index( n )];
        continue;
      }

      /* continue if cut is not in the cover */
      if ( node_match[index].map_refs[2] == 0u )
        continue;

      unsigned phase = ( node_data.best_supergate[0] != nullptr ) ? 0 : 1;

      if ( node_data.same_match || node_data.map_refs[phase] > 0 )
      {
        power += switch_activity[ntk.node_to_index( n )];

        if ( node_data.same_match && node_data.map_refs[phase ^ 1] > 0 )
          power += switch_activity[ntk.node_to_index( n )];
      }

      phase = phase ^ 1;
      if ( !node_data.same_match && node_data.map_refs[phase] > 0 )
      {
        power += switch_activity[ntk.node_to_index( n )];
      }
    }

    return power;
  }

  double compute_match_wirelength(cut_t const& cut,
                                  node_position const& gate_position,
                                  uint8_t best_phase) 
  {
    double worst_wl = 0.0f;
    auto ctr = 0u;
    for (auto& c : cut) {
      double wl_temp = std::abs(gate_position.x_coordinate -
                                node_match[c].position[(best_phase >> ctr) & 1].x_coordinate) +
                       std::abs(gate_position.y_coordinate -
                                node_match[c].position[(best_phase >> ctr) & 1].y_coordinate) +
                       node_match[c].wirelength[(best_phase >> ctr) & 1];
      worst_wl = std::max(wl_temp, worst_wl);
    //   std::cout << "wl_temp: " << wl_temp << "       ";
      ++ctr;
    }
    // std::cout << std::endl;
    return worst_wl;
  }

  double compute_match_total_wirelength(cut_t const& cut,
                                  node_position const& gate_position,
                                  uint8_t best_phase)  {
    double worst_wl = 0.0f;
    auto ctr = 0u;
    for (auto& c : cut) {
      double wl_temp = std::abs(gate_position.x_coordinate -
                                node_match[c].position[(best_phase >> ctr) & 1].x_coordinate) +
                       std::abs(gate_position.y_coordinate -
                                node_match[c].position[(best_phase >> ctr) & 1].y_coordinate) +
                       node_match[c].total_wirelength[(best_phase >> ctr) & 1];
      // auto& pin_data = node_match[c];
      // worst_wl += (wl_temp / pin_data.est_refs[2]);
      ++ctr;
    }
    return worst_wl;
  }

  double compute_wirelength(cut_t const& cut) 
  {
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

  double weight_w_d(double wirelength_t, double total_wirelength_t, double delay_t) {
    double wires = ((1 - ps.trade_off) * wirelength_t) +
                   (ps.trade_off * total_wirelength_t);
    return (0.8 * wires + 0.2 * delay_t) / 2;
  }

  void print_node_match() 
  {
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
                << std::endl;
      std::cout << "total wirelength: " << node_match[i].total_wirelength[0]
                << ", " << node_match[i].total_wirelength[1] << ", "
                << node_match[i].total_wirelength[2] << std::endl
                << std::endl;
    }
    std::cout << "area, delay, wirelength, total_wirelength, iteration: "
              << area << ", " << delay << ", " << wirelength << ", "
              << total_wirelength << ", " << iteration << std::endl
              << std::endl;
  }

protected:
  Ntk const& ntk;
  tech_library<NInputs, Configuration> const& library;
  std::vector<node_position> np;
  map_params const& ps;
  map_stats& st;

  uint32_t iteration{ 0 };         /* current mapping iteration */
  double delay{ 0.0f };            /* current delay of the mapping */
  double area{ 0.0f };             /* current area of the mapping */
  double wirelength{ 0.0f };       /* current wirelength of the mapping */
  double total_wirelength{ 0.0f }; /* current total wirelength of the mapping */    
  double congestion{ 0.0f };       /* current congestion of the mapping */
  const float epsilon{ 0.005f };   /* epsilon */

  /* lib inverter info */
  float lib_inv_area;
  float lib_inv_delay;
  uint32_t lib_inv_id;

  /* lib buffer info */
  float lib_buf_area;
  float lib_buf_delay;
  uint32_t lib_buf_id;

  std::vector<node<Ntk>> top_order;
  std::vector<node_match_phy> node_match;
  match_map matches;
  std::vector<float> switch_activity;
  network_cuts_t cuts;
  std::unordered_map<signal<klut_network>, index_phase_pair> res2ntk;
  std::vector<std::vector<signal<klut_network>>> _binding_roots;
  std::vector<node_position> match_position;

  // Map for congestion awareness
  std::unique_ptr<phyLS::RUDY<map_ntk_t>> rudy_map;
};
} // namespace detail

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
  detail::phy_map_impl<Ntk, CutSize, CutData, NInputs, Configuration> p{ntk, library, np, ps, st};
  auto res = p.rudy_map_test();

  st.time_total = st.time_mapping + st.cut_enumeration_st.time_total;
  if (ps.verbose && !st.mapping_error) st.report();

  if (pst) *pst = st;

  return res;
}
} // namespace mockturtle