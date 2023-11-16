/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/*!
  \file mg.hpp
  \brief Matrix logic network implementation
  \brief Inspired from aig.hpp in mockturtle
  \author Homyoung
*/

#pragma once

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operators.hpp>
#include <kitty/partial_truth_table.hpp>
#include <memory>
#include <mockturtle/networks/detail/foreach.hpp>
#include <mockturtle/networks/events.hpp>
#include <mockturtle/networks/storage.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/utils/algorithm.hpp>
#include <optional>
#include <stack>
#include <string>

namespace mockturtle {

/*! \brief Hash function for AIGs (from ABC) */
template <class Node>
struct mg_hash {
  uint64_t operator()(Node const& n) const {
    uint64_t seed = -2011;
    seed += n.children[0].index * 7937;
    seed += n.children[1].index * 2971;
    seed += n.children[0].weight * 911;
    seed += n.children[1].weight * 353;
    return seed;
  }
};

using mg_storage = storage<regular_node<2, 2, 1>, empty_storage_data,
                           mg_hash<regular_node<2, 2, 1>>>;

class mg_network {
 public:
#pragma region Types and constructors
  static constexpr auto min_fanin_size = 2u;
  static constexpr auto max_fanin_size = 2u;

  using base_type = mg_network;
  using storage = std::shared_ptr<mg_storage>;
  using node = uint64_t;

  struct signal {
    signal() = default;

    signal(uint64_t index, uint64_t complement)
        : complement(complement), index(index) {}

    explicit signal(uint64_t data) : data(data) {}

    signal(mg_storage::node_type::pointer_type const& p)
        : complement(p.weight), index(p.index) {}

    union {
      struct {
        uint64_t complement : 1;
        uint64_t index : 63;
      };
      uint64_t data;
    };

    signal operator!() const { return signal(data ^ 1); }

    signal operator+() const { return {index, 0}; }

    signal operator-() const { return {index, 1}; }

    signal operator^(bool complement) const {
      return signal(data ^ (complement ? 1 : 0));
    }

    bool operator==(signal const& other) const { return data == other.data; }

    bool operator!=(signal const& other) const { return data != other.data; }

    bool operator<(signal const& other) const { return data < other.data; }

    operator mg_storage::node_type::pointer_type() const {
      return {index, complement};
    }

#if __cplusplus > 201703L
    bool operator==(mg_storage::node_type::pointer_type const& other) const {
      return data == other.data;
    }
#endif
  };

  mg_network()
      : _storage(std::make_shared<mg_storage>()),
        _events(std::make_shared<decltype(_events)::element_type>()) {}

  mg_network(std::shared_ptr<mg_storage> storage)
      : _storage(storage),
        _events(std::make_shared<decltype(_events)::element_type>()) {}

  mg_network clone() const { return {std::make_shared<mg_storage>(*_storage)}; }
#pragma endregion

#pragma region Primary I / O and constants
  signal get_constant(bool value) const {
    return {0, static_cast<uint64_t>(value ? 1 : 0)};
  }

  signal create_pi() {
    const auto index = _storage->nodes.size();
    auto& node = _storage->nodes.emplace_back();
    node.children[0].data = node.children[1].data = _storage->inputs.size();
    _storage->inputs.emplace_back(index);
    return {index, 0};
  }

  uint32_t create_po(signal const& f) {
    /* increase ref-count to children */
    _storage->nodes[f.index].data[0].h1++;
    auto const po_index = _storage->outputs.size();
    _storage->outputs.emplace_back(f.index, f.complement);
    return static_cast<uint32_t>(po_index);
  }

  bool is_combinational() const { return true; }

  bool is_constant(node const& n) const { return n == 0; }

  bool is_ci(node const& n) const {
    return _storage->nodes[n].children[0].data ==
           _storage->nodes[n].children[1].data;
  }

  bool is_pi(node const& n) const {
    return _storage->nodes[n].children[0].data ==
               _storage->nodes[n].children[1].data &&
           !is_constant(n);
  }

  bool constant_value(node const& n) const {
    (void)n;
    return false;
  }
#pragma endregion

#pragma region Create unary functions
  signal create_buf(signal const& a) { return a; }
  signal create_not(signal const& a) { return !a; }
#pragma endregion

#pragma region Create binary functions
  signal create_node(signal a, signal b, uint32_t literal) {
    storage::element_type::node_type node;
    node.children[0] = a;
    node.children[1] = b;
    node.data[1].h1 = literal;

    /* structural hashing */
    const auto it = _storage->hash.find(node);
    if (it != _storage->hash.end()) return {it->second, 0};

    const auto index = _storage->nodes.size();

    if (index >= .9 * _storage->nodes.capacity()) {
      _storage->nodes.reserve(static_cast<uint64_t>(3.1415f * index));
      _storage->hash.reserve(static_cast<uint64_t>(3.1415f * index));
    }

    _storage->nodes.push_back(node);
    _storage->hash[node] = index;

    /* increase ref-count to children */
    _storage->nodes[a.index].data[0].h1++;
    _storage->nodes[b.index].data[0].h1++;

    for (auto const& fn : _events->on_add) (*fn)(index);

    return {index, 0};
  }

  signal create_0(signal a, signal b) { return get_constant(false); }
  signal create_1(signal a, signal b) {
    if (a.index > b.index) std::swap(a, b);
    if (a.index == b.index) {
      return create_not(a);
    } else if (a.index == 0) {
      return create_not(b);
    } else if (a.index == 1) {
      return get_constant(false);
    } else if (b.index == 0) {
      return create_not(a);
    } else if (b.index == 1) {
      return get_constant(false);
    }
    return create_node(a, b, 1);
  }
  signal create_2(signal a, signal b) { return create_node(a, b, 2); }
  signal create_not(signal a, signal b) { return create_node(a, b, 3); }
  signal create_and(signal a, signal b) { return create_node(a, b, 4); }
  signal create_nand(signal a, signal b) { return create_node(a, b, 5); }
  signal create_or(signal a, signal b) { return create_node(a, b, 6); }
  signal create_7(signal a, signal b) { return create_node(a, b, 7); }
  signal create_lt(signal a, signal b) { return create_node(a, b, 8); }
  signal create_9(signal a, signal b) { return create_node(a, b, 9); }
  signal create_a(signal a, signal b) { return create_node(a, b, 10); }
  signal create_le(signal a, signal b) { return create_node(a, b, 11); }
  signal create_xor(signal a, signal b) { return create_node(a, b, 12); }
  signal create_d(signal a, signal b) { return create_node(a, b, 13); }
  signal create_e(signal a, signal b) { return create_node(a, b, 14); }
  signal create_f(signal a, signal b) { return get_constant(true); }

#pragma endregion

#pragma region Createy ternary functions

#pragma endregion

#pragma region Create nary functions

#pragma endregion

#pragma region Create arbitrary functions
  signal clone_node(mg_network const& other, node const& source,
                    std::vector<signal> const& children) {
    (void)other;
    (void)source;
    assert(children.size() == 2u);
    const auto tt = other._storage->nodes[source].data[1].h1;
    return create_node(children[0u], children[1u], tt);
  }
#pragma endregion

#pragma region Restructuring
  void substitute_node(node const& old_node, signal const& new_signal) {
    /* find all parents from old_node */
    for (auto i = 0u; i < _storage->nodes.size(); ++i) {
      auto& n = _storage->nodes[i];
      for (auto& child : n.children) {
        if (child == old_node) {
          std::vector<signal> old_children(n.children.size());
          std::transform(n.children.begin(), n.children.end(),
                         old_children.begin(), [](auto c) { return c.index; });
          child = new_signal;

          // increment fan-out of new node
          _storage->nodes[new_signal].data[0].h1++;

          for (auto const& fn : _events->on_modified) {
            (*fn)(i, old_children);
          }
        }
      }
    }

    /* check outputs */
    for (auto& output : _storage->outputs) {
      if (output == old_node) {
        output = new_signal;

        // increment fan-out of new node
        _storage->nodes[new_signal].data[0].h1++;
      }
    }

    // reset fan-out of old node
    _storage->nodes[old_node].data[0].h1 = 0;
  }

  inline bool is_dead(node const& n) const { return false; }
#pragma endregion

#pragma region Structural properties
  auto size() const { return static_cast<uint32_t>(_storage->nodes.size()); }

  auto num_cis() const {
    return static_cast<uint32_t>(_storage->inputs.size());
  }

  auto num_cos() const {
    return static_cast<uint32_t>(_storage->outputs.size());
  }

  auto num_pis() const {
    return static_cast<uint32_t>(_storage->inputs.size());
  }

  auto num_pos() const {
    return static_cast<uint32_t>(_storage->outputs.size());
  }

  auto num_gates() const {
    return static_cast<uint32_t>(_storage->hash.size());
  }

  uint32_t fanin_size(node const& n) const {
    if (is_constant(n) || is_ci(n)) return 0;
    return 2;
  }

  uint32_t fanout_size(node const& n) const {
    return _storage->nodes[n].data[0].h1 & UINT32_C(0x7FFFFFFF);
  }

  uint32_t incr_fanout_size(node const& n) const {
    return _storage->nodes[n].data[0].h1++ & UINT32_C(0x7FFFFFFF);
  }

  uint32_t decr_fanout_size(node const& n) const {
    return --_storage->nodes[n].data[0].h1 & UINT32_C(0x7FFFFFFF);
  }
#pragma endregion

#pragma region Functional properties
//   kitty::dynamic_truth_table node_function(const node& n) const {
//     return _storage->nodes[n].data[1].h1;
//   }
#pragma endregion

#pragma region Nodes and signals
  node get_node(signal const& f) const { return f.index; }

  signal make_signal(node const& n) const { return signal(n, 0); }

  bool is_complemented(signal const& f) const { return f.complement; }

  uint32_t node_to_index(node const& n) const {
    return static_cast<uint32_t>(n);
  }

  node index_to_node(uint32_t index) const { return index; }

  node ci_at(uint32_t index) const {
    assert(index < _storage->inputs.size());
    return *(_storage->inputs.begin() + index);
  }

  signal co_at(uint32_t index) const {
    assert(index < _storage->outputs.size());
    return *(_storage->outputs.begin() + index);
  }

  node pi_at(uint32_t index) const {
    assert(index < _storage->inputs.size());
    return *(_storage->inputs.begin() + index);
  }

  signal po_at(uint32_t index) const {
    assert(index < _storage->outputs.size());
    return *(_storage->outputs.begin() + index);
  }

  uint32_t ci_index(node const& n) const {
    assert(_storage->nodes[n].children[0].data ==
           _storage->nodes[n].children[1].data);
    return static_cast<uint32_t>(_storage->nodes[n].children[0].data);
  }

  uint32_t co_index(signal const& s) const {
    uint32_t i = -1;
    foreach_co([&](const auto& x, auto index) {
      if (x == s) {
        i = index;
        return false;
      }
      return true;
    });
    return i;
  }

  uint32_t pi_index(node const& n) const {
    assert(_storage->nodes[n].children[0].data ==
           _storage->nodes[n].children[1].data);
    return static_cast<uint32_t>(_storage->nodes[n].children[0].data);
  }

  uint32_t po_index(signal const& s) const {
    uint32_t i = -1;
    foreach_po([&](const auto& x, auto index) {
      if (x == s) {
        i = index;
        return false;
      }
      return true;
    });
    return i;
  }
#pragma endregion

#pragma region Node and signal iterators
  template <typename Fn>
  void foreach_node(Fn&& fn) const {
    auto r = range<uint64_t>(_storage->nodes.size());
    detail::foreach_element_if(
        r.begin(), r.end(), [this](auto n) { return !is_dead(n); }, fn);
  }

  template <typename Fn>
  void foreach_ci(Fn&& fn) const {
    detail::foreach_element(_storage->inputs.begin(), _storage->inputs.end(),
                            fn);
  }

  template <typename Fn>
  void foreach_co(Fn&& fn) const {
    detail::foreach_element(_storage->outputs.begin(), _storage->outputs.end(),
                            fn);
  }

  template <typename Fn>
  void foreach_pi(Fn&& fn) const {
    detail::foreach_element(_storage->inputs.begin(), _storage->inputs.end(),
                            fn);
  }

  template <typename Fn>
  void foreach_po(Fn&& fn) const {
    detail::foreach_element(_storage->outputs.begin(), _storage->outputs.end(),
                            fn);
  }

  template <typename Fn>
  void foreach_gate(Fn&& fn) const {
    auto r = range<uint64_t>(
        1u, _storage->nodes.size()); /* start from 1 to avoid constant */
    detail::foreach_element_if(
        r.begin(), r.end(), [this](auto n) { return !is_ci(n) && !is_dead(n); },
        fn);
  }

  template <typename Fn>
  void foreach_fanin(node const& n, Fn&& fn) const {
    if (n == 0 || is_ci(n)) return;

    static_assert(detail::is_callable_without_index_v<Fn, signal, bool> ||
                  detail::is_callable_with_index_v<Fn, signal, bool> ||
                  detail::is_callable_without_index_v<Fn, signal, void> ||
                  detail::is_callable_with_index_v<Fn, signal, void>);

    /* we don't use foreach_element here to have better performance */
    if constexpr (detail::is_callable_without_index_v<Fn, signal, bool>) {
      if (!fn(signal{_storage->nodes[n].children[0]})) return;
      fn(signal{_storage->nodes[n].children[1]});
    } else if constexpr (detail::is_callable_with_index_v<Fn, signal, bool>) {
      if (!fn(signal{_storage->nodes[n].children[0]}, 0)) return;
      fn(signal{_storage->nodes[n].children[1]}, 1);
    } else if constexpr (detail::is_callable_without_index_v<Fn, signal,
                                                             void>) {
      fn(signal{_storage->nodes[n].children[0]});
      fn(signal{_storage->nodes[n].children[1]});
    } else if constexpr (detail::is_callable_with_index_v<Fn, signal, void>) {
      fn(signal{_storage->nodes[n].children[0]}, 0);
      fn(signal{_storage->nodes[n].children[1]}, 1);
    }
  }
#pragma endregion

#pragma region Value simulation
  template <typename Iterator>
  iterates_over_t<Iterator, bool> compute(node const& n, Iterator begin,
                                          Iterator end) const {
    (void)end;

    assert(n != 0 && !is_ci(n));

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];

    auto v1 = *begin++;
    auto v2 = *begin++;

    if (c2.index == 0) {
      return ~v1 ^ c1.weight;
    } else {
      return (~v1 ^ c1.weight) || (v2 ^ c2.weight);
    }
  }

  template <typename Iterator>
  iterates_over_truth_table_t<Iterator> compute(node const& n, Iterator begin,
                                                Iterator end) const {
    (void)end;

    assert(n != 0 && !is_ci(n));

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];

    auto tt1 = *begin++;
    auto tt2 = *begin++;

    if (c2.index == 0) {
      return c1.weight ? tt1 : ~tt1;
    } else {
      return (c1.weight ? tt1 : ~tt1) | (c2.weight ? ~tt2 : tt2);
    }
  }
#pragma endregion

#pragma region Custom node values
  void clear_values() const {
    std::for_each(_storage->nodes.begin(), _storage->nodes.end(),
                  [](auto& n) { n.data[0].h2 = 0; });
  }

  auto value(node const& n) const { return _storage->nodes[n].data[0].h2; }

  void set_value(node const& n, uint32_t v) const {
    _storage->nodes[n].data[0].h2 = v;
  }

  auto incr_value(node const& n) const {
    return _storage->nodes[n].data[0].h2++;
  }

  auto decr_value(node const& n) const {
    return --_storage->nodes[n].data[0].h2;
  }
#pragma endregion

#pragma region Visited flags
  void clear_visited() const {
    std::for_each(_storage->nodes.begin(), _storage->nodes.end(),
                  [](auto& n) { n.data[1].h1 = 0; });
  }

  auto visited(node const& n) const { return _storage->nodes[n].data[1].h1; }

  void set_visited(node const& n, uint32_t v) const {
    _storage->nodes[n].data[1].h1 = v;
  }

  uint32_t trav_id() const { return _storage->trav_id; }

  void incr_trav_id() const { ++_storage->trav_id; }
#pragma endregion

#pragma region General methods
  auto& events() const { return *_events; }
#pragma endregion

 public:
  std::shared_ptr<mg_storage> _storage;
  std::shared_ptr<network_events<base_type>> _events;
};

}  // namespace mockturtle

namespace std {

template <>
struct hash<mockturtle::mg_network::signal> {
  uint64_t operator()(mockturtle::mg_network::signal const& s) const noexcept {
    uint64_t k = s.data;
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return k;
  }
}; /* hash */

}  // namespace std
