/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file abc_gia.hpp
 *
 * @brief  ABC GIA network
 *
 * @author Jiaxiang Pan, Jun zhu
 * @since  2024/09/20
 */

#ifndef ABC_GIA_HPP
#define ABC_GIA_HPP

#include <cassert>
#include <iostream>

#include <mockturtle/networks/detail/foreach.hpp>
#include <kitty/dynamic_truth_table.hpp>

namespace mockturtle {

class gia_network;
  
class gia_signal {
  friend class gia_network;

public:
  explicit gia_signal() = default;
  explicit gia_signal(pabc::Gia_Obj_t * obj) : obj_(obj) {}
 
  gia_signal operator!() const { return gia_signal(pabc::Gia_Not(obj_)); }
  gia_signal operator+() const { return gia_signal(pabc::Gia_Regular(obj_)); }
  gia_signal operator-() const { return gia_signal(pabc::Gia_Not(pabc::Gia_Regular(obj_))); }
 
  bool operator==(const gia_signal& other) const { return obj_ == other.obj_; }
  bool operator!=(const gia_signal& other) const { return !operator==(other); }
  bool operator<(const gia_signal& other) const { return obj_ < other.obj_; }

  pabc::Gia_Obj_t * obj() const { return obj_; }
  
private:
  pabc::Gia_Obj_t * obj_;
};
  
class gia_network {
public:
  static constexpr auto min_fanin_size = 2u;
  static constexpr auto max_fanin_size = 2u;

  using base_type = gia_network;
  using node = int;
  using signal = gia_signal;
  using storage = pabc::Gia_Man_t*;
  
  gia_network(int size)
    : gia_(pabc::Gia_ManStart(size)) /* doesn't automatically resize? */
  {}

  gia_network(pabc::Gia_Man_t * gia_ntk)
    : gia_(gia_ntk) /* doesn't automatically resize? */
  {}

  /* network does not implement constant value */
  bool constant_value(node n) const { (void)n; return false; }

  /* each node implements AND function */
  kitty::dynamic_truth_table node_function(node n) const { (void)n; kitty::dynamic_truth_table tt(2); tt._bits[0] = 0x8; return tt; }

  
  signal get_constant(bool value) const {
    return value ? signal(pabc::Gia_ManConst1(gia_)) : signal(pabc::Gia_ManConst0(gia_));
  }
  
  signal create_pi() {
    return signal(pabc::Gia_ManObj(gia_, pabc::Abc_Lit2Var(pabc::Gia_ManAppendCi(gia_))));
  }

  void create_po(const signal& f) {
    /* po_node = */pabc::Gia_ManAppendCo(gia_, pabc::Gia_Obj2Lit(gia_, f.obj()));
  }

  signal create_not(const signal& f) {
    return !f;
  }
  
  signal create_and(const signal& f, const signal& g) {
    return signal(pabc::Gia_ManObj(gia_, pabc::Abc_Lit2Var(pabc::Gia_ManAppendAnd2(gia_, pabc::Gia_Obj2Lit(gia_, f.obj()), pabc::Gia_Obj2Lit(gia_, g.obj())))));
  }

  bool is_constant(node n) const {
    return n == 0;
  }

  node get_node(const signal& f) const {
    return Gia_ObjId(gia_, f.obj());
  }

  bool is_pi(node const& n) const {
    return pabc::Gia_ObjIsPi(gia_, pabc::Gia_ManObj(gia_, n));
  }

  bool is_complemented(const signal& f) const {
    return Gia_IsComplement(f.obj());
  }

  template<typename Fn>
  void foreach_pi(Fn&& fn) const {
    pabc::Gia_Obj_t * pObj;
    for (int i = 0; (i < pabc::Gia_ManPiNum(gia_)) && ((pObj) = pabc::Gia_ManCi(gia_, i)); ++i) {
      fn(Gia_ObjId(gia_, pObj));
    }
  }

  template<typename Fn>
  void foreach_po(Fn&& fn) const {
    pabc::Gia_Obj_t * pObj, * pFiObj;
    for (int i = 0; (i < pabc::Gia_ManPoNum(gia_)) && ((pObj) = pabc::Gia_ManCo(gia_, i)); ++i) {
      pFiObj = pabc::Gia_ObjFanin0(pObj);
      fn(pabc::Gia_ObjFaninC0(pObj) ? !signal(pFiObj) : signal(pFiObj));
    }
  }

  template<typename Fn>
  void foreach_gate(Fn&& fn) const {
    pabc::Gia_Obj_t * pObj;
    for (int i = 0; i < pabc::Gia_ManObjNum(gia_) && ((pObj) = pabc::Gia_ManObj(gia_, i)); ++i) {
      if (pabc::Gia_ObjIsAnd(pObj)) {
        fn(Gia_ObjId(gia_, pObj));
      }
    }
  }  

  template<typename Fn>
  void foreach_node(Fn&& fn) const {
    pabc::Gia_Obj_t * pObj;
    for (int i = 0; i < pabc::Gia_ManObjNum(gia_) && ((pObj) = pabc::Gia_ManObj(gia_, i)); ++i) {
      fn(signal(pObj));
    }
  }  

  template<typename Fn>
  void foreach_fanin(node n, Fn&& fn) const {
    static_assert( detail::is_callable_without_index_v<Fn, signal, bool> ||
                   detail::is_callable_with_index_v<Fn, signal, bool> ||
                   detail::is_callable_without_index_v<Fn, signal, void> ||
                   detail::is_callable_with_index_v<Fn, signal, void> );

    if (n == 0 || is_pi(n)) { return; }    

    pabc::Gia_Obj_t * pObj = pabc::Gia_ManObj(gia_, n);
    if constexpr ( detail::is_callable_without_index_v<Fn, signal, bool> )
    {
      if (!fn(signal(pabc::Gia_ObjFaninC0(pObj) ? Gia_Not(pabc::Gia_ObjFanin0(pObj)) : pabc::Gia_ObjFanin0(pObj)))) {
        return;
      }
      fn(signal(pabc::Gia_ObjFaninC1(pObj) ? Gia_Not(pabc::Gia_ObjFanin1(pObj)) : pabc::Gia_ObjFanin1(pObj)));
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, signal, bool> )
    {
      if (!fn(signal(pabc::Gia_ObjFaninC0(pObj) ? Gia_Not(pabc::Gia_ObjFanin0(pObj)) : pabc::Gia_ObjFanin0(pObj))), 0) {
        return;
      }
      fn(signal(pabc::Gia_ObjFaninC1(pObj) ? Gia_Not(pabc::Gia_ObjFanin1(pObj)) : pabc::Gia_ObjFanin1(pObj)), 1);
    }
    else if constexpr ( detail::is_callable_without_index_v<Fn, signal, void> )
    {
      fn(signal(pabc::Gia_ObjFaninC0(pObj) ? Gia_Not(pabc::Gia_ObjFanin0(pObj)) : pabc::Gia_ObjFanin0(pObj)));
      fn(signal(pabc::Gia_ObjFaninC1(pObj) ? Gia_Not(pabc::Gia_ObjFanin1(pObj)) : pabc::Gia_ObjFanin1(pObj)));
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, signal, void> )
    {
      fn(signal(pabc::Gia_ObjFaninC0(pObj) ? Gia_Not(pabc::Gia_ObjFanin0(pObj)) : pabc::Gia_ObjFanin0(pObj)), 0);
      fn(signal(pabc::Gia_ObjFaninC1(pObj) ? Gia_Not(pabc::Gia_ObjFanin1(pObj)) : pabc::Gia_ObjFanin1(pObj)), 1);
    }
  }  

  int literal(const signal& f) const
  {
    return (pabc::Gia_ObjId(gia_, f.obj()) << 1) + pabc::Gia_IsComplement(f.obj());
  }
  
  int node_to_index(node n) const
  {
    return n;
  }

  auto num_pis() const { return pabc::Gia_ManPiNum(gia_); }
  auto num_pos() const { return pabc::Gia_ManPoNum(gia_); }
  auto num_gates() const { return pabc::Gia_ManAndNum(gia_); }
  auto num_levels() const { return pabc::Gia_ManLevelNum(gia_); }
  auto size() const { return pabc::Gia_ManObjNum(gia_); }

  bool load_rc() {
    pabc::Abc_Frame_t * abc = pabc::Abc_FrameGetGlobalFrame();
    const int success = pabc::Cmd_CommandExecute(abc, default_rc);
    if (success != 0) {
      printf("syntax error in script\n");
    }
    return success == 0;
  }

  bool run_opt_script(const std::string &script) {
    pabc::Gia_Man_t * gia = pabc::Gia_ManDup(gia_);
    pabc::Abc_Frame_t * abc = pabc::Abc_FrameGetGlobalFrame();
    pabc::Abc_FrameUpdateGia(abc, gia);

    const int success = pabc::Cmd_CommandExecute(abc, script.c_str());
    if (success != 0) {
      printf("syntax error in script\n");
    }

    pabc::Gia_Man_t * new_gia = pabc::Abc_FrameGetGia(abc);
    pabc::Gia_ManStop(gia_);
    gia_ = new_gia;
    
    return success == 0;
  }

  const pabc::Gia_Man_t * get_gia() const { return gia_; };

private:
  const char * default_rc =
    "alias b balance;\n"
    "alias rw rewrite;\n"
    "alias rwz rewrite -z;\n"
    "alias rf refactor;\n"
    "alias rfz refactor -z;\n"
    "alias rs resub;\n"
    "alias rsz resub -z;\n"
    "alias &r2rs '&put; b; rs -K 6; rw; rs -K 6 -N 2; rf; rs -K 8; b; rs -K 8 -N 2; rw; rs -K 10; rwz; rs -K 10 -N 2; b; rs -K 12; rfz; rs -K 12 -N 2; rwz; b; &get';\n"
    "alias &c2rs '&put; b -l; rs -K 6 -l; rw -l; rs -K 6 -N 2 -l; rf -l; rs -K 8 -l; b -l; rs -K 8 -N 2 -l; rw -l; rs -K 10 -l; rwz -l; rs -K 10 -N 2 -l; b -l; rs -K 12 -l; rfz -l; rs -K 12 -N 2 -l; rwz -l; b -l; &get';\n"
    "alias compress2rs 'b -l; rs -K 6 -l; rw -l; rs -K 6 -N 2 -l; rf -l; rs -K 8 -l; b -l; rs -K 8 -N 2 -l; rw -l; rs -K 10 -l; rwz -l; rs -K 10 -N 2 -l; b -l; rs -K 12 -l; rfz -l; rs -K 12 -N 2 -l; rwz -l; b -l';\n"
    "alias resyn2rs 'b; rs -K 6; rw; rs -K 6 -N 2; rf; rs -K 8; b; rs -K 8 -N 2; rw; rs -K 10; rwz; rs -K 10 -N 2; b; rs -K 12; rfz; rs -K 12 -N 2; rwz; b;'\n"
    "alias resyn2rs2 'b; rs -K 6; rw; rs -K 6 -N 2; rs -K 8; b; rs -K 8 -N 2; rw; rs -K 10; rwz; rs -K 10 -N 2; b; rs -K 12; rs -K 12 -N 2; rwz; b;'\n";

private:
  pabc::Gia_Man_t *gia_;
}; // gia_network

} // mockturtle

#endif
