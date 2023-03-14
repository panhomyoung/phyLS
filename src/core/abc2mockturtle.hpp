/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file abc2mockturtle.hpp
 *
 * @brief convert format between abc and mockturtle
 *
 * @author Homyoung
 * @since  2023/02/27
 */

#ifndef ABC2MOCKTURTLE_HPP
#define ABC2MOCKTURTLE_HPP

#include <base/abc/abc.h>
#include <base/io/ioAbc.h>

#include <map>
#include <mockturtle/mockturtle.hpp>
#include <string>
#include <vector>

namespace phyLS {

mockturtle::aig_network aig_new;
pabc::Abc_Ntk_t* abc_ntk_;
std::map<int, std::string> pi_names;
std::map<int, std::string> po_names;

mockturtle::aig_network& abc2mockturtle_a(pabc::Abc_Ntk_t* abc_ntk_new) {
  // Convert abc aig network to the aig network in also
  pabc::Abc_Ntk_t* abc_ntk_aig;
  abc_ntk_aig = abc_ntk_new;
  mockturtle::aig_network aig;
  using sig = mockturtle::aig_network::signal;
  std::map<int, sig> signals;
  signals.clear();
  pi_names.clear();
  po_names.clear();

  // create also network
  // process the also network primary input
  int i;
  pabc::Abc_Obj_t* pObj1;
  for (i = 0; (i < pabc::Abc_NtkPiNum(abc_ntk_aig)) &&
              (((pObj1) = pabc::Abc_NtkPi(abc_ntk_aig, i)), 1);
       i++) {
    sig pi = aig.create_pi();
    signals.insert(std::pair<int, sig>(pObj1->Id, pi));
    unsigned int a = pi.index;
    pi_names.insert(std::pair<int, std::string>(a, pabc::Abc_ObjName(pObj1)));
  }

  // process the also network node
  int j, k;
  pabc::Abc_Obj_t *pNode, *pFanin1;
  for (i = 0; (i < pabc::Vec_PtrSize((abc_ntk_aig)->vObjs)) &&
              (((pNode) = pabc::Abc_NtkObj(abc_ntk_aig, i)), 1);
       i++) {
    if ((pNode) == NULL || !pabc::Abc_ObjIsNode(pNode)) {
    } else {
      std::vector<sig> children;
      children.clear();

      if (pabc::Abc_AigNodeIsConst(pabc::Abc_ObjFanin0(pNode)))
        children.push_back((pNode->fCompl0)
                               ? aig.create_not(aig.get_constant(true))
                               : aig.get_constant(true));
      else
        children.push_back(
            (pNode->fCompl0)
                ? aig.create_not(signals[pabc::Abc_ObjFaninId0(pNode)])
                : signals[pabc::Abc_ObjFaninId0(pNode)]);

      if (pabc::Abc_AigNodeIsConst(pabc::Abc_ObjFanin1(pNode)))
        children.push_back((pNode->fCompl1)
                               ? aig.create_not(aig.get_constant(true))
                               : aig.get_constant(true));
      else
        children.push_back(
            (pNode->fCompl1)
                ? aig.create_not(signals[pabc::Abc_ObjFaninId1(pNode)])
                : signals[pabc::Abc_ObjFaninId1(pNode)]);

      assert(children.size() == 2u);
      signals.insert(std::pair<int, sig>(
          pNode->Id, aig.create_and(children[0], children[1])));
    }
  }

  // process the also network primary output
  int m, n, n1;
  pabc::Abc_Obj_t *pObj2, *pFanin2;
  for (i = 0; (i < pabc::Abc_NtkPoNum(abc_ntk_aig)) &&
              (((pObj2) = pabc::Abc_NtkPo(abc_ntk_aig, i)), 1);
       i++) {
    if (pabc::Abc_AigNodeIsConst(pabc::Abc_ObjFanin0(pObj2))) {
      auto const o = (pObj2->fCompl0) ? aig.create_not(aig.get_constant(true))
                                      : aig.get_constant(true);
      aig.create_po(o);
    } else {
      auto const o = (pObj2->fCompl0)
                         ? aig.create_not(signals[pabc::Abc_ObjFaninId0(pObj2)])
                         : signals[pabc::Abc_ObjFaninId0(pObj2)];
      aig.create_po(o);
    }
    po_names.insert(std::pair<int, std::string>(i, pabc::Abc_ObjName(pObj2)));
  }

  aig_new = aig;
  return aig_new;
}

pabc::Abc_Ntk_t* mockturtle2abc_x(const mockturtle::xmg_network& xmg) {
  // Convert xmg network to the netlist network in abc
  const int MAX = 3;
  bool const0_created = false;
  std::string constant_name = "1\'b0";
  // Initialize the abc network
  abc_ntk_ = pabc::Abc_NtkAlloc(pabc::ABC_NTK_NETLIST, pabc::ABC_FUNC_SOP, 1);
  abc_ntk_->pName = strdup("netlist");

  mockturtle::topo_view topo_xmg{xmg};

  // create abc network
  // process the abc network primary input
  topo_xmg.foreach_pi([&](auto const& n, auto index) {
    const auto index1 = topo_xmg.node_to_index(n);
    unsigned int a = index1;
    std::string input_name = pi_names[a];
    pabc::Io_ReadCreatePi(abc_ntk_, (char*)(input_name.c_str()));
  });

  // process the abc network node
  topo_xmg.foreach_node([&](auto n) {
    if (topo_xmg.is_constant(n) || topo_xmg.is_pi(n)) return true;

    auto func = topo_xmg.node_function(n);
    std::string node_name = std::to_string(topo_xmg.node_to_index(n));

    std::vector<std::string> children;
    children.clear();
    char* pNamesIn[MAX];
    topo_xmg.foreach_fanin(n, [&](auto const& f) {
      auto index = topo_xmg.node_to_index(topo_xmg.get_node(f));
      std::string fanin_name = std::to_string(index);

      if (topo_xmg.is_pi(topo_xmg.get_node(f))) {
        unsigned int b = index;
        children.push_back(pi_names[b]);
      } else if (topo_xmg.is_constant(topo_xmg.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children.push_back(constant_name);
      } else
        children.push_back(fanin_name);
    });

    for (size_t i = 0; i < children.size(); i++)
      pNamesIn[i] = (char*)(children[i].c_str());

    pabc::Abc_Obj_t* pNode;
    pNode = pabc::Io_ReadCreateNode(abc_ntk_, (char*)(node_name.c_str()),
                                    pNamesIn, topo_xmg.fanin_size(n));
    std::string abc_func = "";
    int count = 0;
    for (auto cube : kitty::isop(func)) {
      topo_xmg.foreach_fanin(n, [&](auto const& f, auto index) {
        if (cube.get_mask(index) && topo_xmg.is_complemented(f))
          cube.flip_bit(index);
      });

      for (auto i = 0u; i < topo_xmg.fanin_size(n); ++i)
        abc_func += (cube.get_mask(i) ? (cube.get_bit(i) ? '1' : '0') : '-');
      abc_func += " 1\n";
      count++;
    }

    pabc::Abc_ObjSetData(
        pNode, pabc::Abc_SopRegister((pabc::Mem_Flex_t*)abc_ntk_->pManFunc,
                                     abc_func.c_str()));
    return true;
  });

  // process the abc network primary output
  std::vector<std::string> children1;
  topo_xmg.foreach_po([&](auto const& f, auto index) {
    auto a = topo_xmg.node_to_index(topo_xmg.get_node(f));
    std::string fanin_name = std::to_string(a);

    pabc::Io_ReadCreatePo(abc_ntk_, (char*)(po_names[index].c_str()));

    if (topo_xmg.is_complemented(f)) {
      children1.clear();
      if (topo_xmg.is_pi(topo_xmg.get_node(f))) {
        unsigned int b = a;
        children1.push_back(pi_names[b]);
      } else if (topo_xmg.is_constant(topo_xmg.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children1.push_back(constant_name);
      } else
        children1.push_back(fanin_name);
      pabc::Io_ReadCreateInv(abc_ntk_, (char*)(children1[0].c_str()),
                             (char*)(po_names[index].c_str()));
    } else {
      children1.clear();
      if (topo_xmg.is_pi(topo_xmg.get_node(f))) {
        unsigned int b = a;
        children1.push_back(pi_names[b]);
      } else if (topo_xmg.is_constant(topo_xmg.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children1.push_back(constant_name);
      } else
        children1.push_back(fanin_name);

      pabc::Io_ReadCreateBuf(abc_ntk_, (char*)(children1[0].c_str()),
                             (char*)(po_names[index].c_str()));
    }
  });

  return abc_ntk_;
}

pabc::Abc_Ntk_t* mockturtle2abc_a(const mockturtle::aig_network& aig) {
  // Convert aig network to the netlist network in abc
  const int MAX = 3;
  bool const0_created = false;
  std::string constant_name = "1\'b0";
  // Initialize the abc network
  abc_ntk_ = pabc::Abc_NtkAlloc(pabc::ABC_NTK_NETLIST, pabc::ABC_FUNC_SOP, 1);
  abc_ntk_->pName = strdup("netlist");

  mockturtle::topo_view topo_aig{aig};

  // create abc network
  // process the abc network primary input
  topo_aig.foreach_pi([&](auto const& n, auto index) {
    const auto index1 = topo_aig.node_to_index(n);
    unsigned int a = index1;
    std::string input_name = pi_names[a];
    pabc::Io_ReadCreatePi(abc_ntk_, (char*)(input_name.c_str()));
  });

  // process the abc network node
  topo_aig.foreach_node([&](auto n) {
    if (topo_aig.is_constant(n) || topo_aig.is_pi(n)) return true;

    auto func = topo_aig.node_function(n);
    std::string node_name = std::to_string(topo_aig.node_to_index(n));

    std::vector<std::string> children;
    children.clear();
    char* pNamesIn[MAX];
    topo_aig.foreach_fanin(n, [&](auto const& f) {
      auto index = topo_aig.node_to_index(topo_aig.get_node(f));
      std::string fanin_name = std::to_string(index);

      if (topo_aig.is_pi(topo_aig.get_node(f))) {
        unsigned int b = index;
        children.push_back(pi_names[b]);
      } else if (topo_aig.is_constant(topo_aig.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children.push_back(constant_name);
      } else
        children.push_back(fanin_name);
    });

    for (size_t i = 0; i < children.size(); i++)
      pNamesIn[i] = (char*)(children[i].c_str());

    pabc::Abc_Obj_t* pNode;
    pNode = pabc::Io_ReadCreateNode(abc_ntk_, (char*)(node_name.c_str()),
                                    pNamesIn, topo_aig.fanin_size(n));
    std::string abc_func = "";
    int count = 0;
    for (auto cube : kitty::isop(func)) {
      topo_aig.foreach_fanin(n, [&](auto const& f, auto index) {
        if (cube.get_mask(index) && topo_aig.is_complemented(f))
          cube.flip_bit(index);
      });

      for (auto i = 0u; i < topo_aig.fanin_size(n); ++i)
        abc_func += (cube.get_mask(i) ? (cube.get_bit(i) ? '1' : '0') : '-');
      abc_func += " 1\n";
      count++;
    }

    pabc::Abc_ObjSetData(
        pNode, pabc::Abc_SopRegister((pabc::Mem_Flex_t*)abc_ntk_->pManFunc,
                                     abc_func.c_str()));
    return true;
  });

  // process the abc network primary output
  std::vector<std::string> children1;
  topo_aig.foreach_po([&](auto const& f, auto index) {
    auto a = topo_aig.node_to_index(topo_aig.get_node(f));
    std::string fanin_name = std::to_string(a);

    pabc::Io_ReadCreatePo(abc_ntk_, (char*)(po_names[index].c_str()));

    if (topo_aig.is_complemented(f)) {
      children1.clear();
      if (topo_aig.is_pi(topo_aig.get_node(f))) {
        unsigned int b = a;
        children1.push_back(pi_names[b]);
      } else if (topo_aig.is_constant(topo_aig.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children1.push_back(constant_name);
      } else
        children1.push_back(fanin_name);
      pabc::Io_ReadCreateInv(abc_ntk_, (char*)(children1[0].c_str()),
                             (char*)(po_names[index].c_str()));
    } else {
      children1.clear();
      if (topo_aig.is_pi(topo_aig.get_node(f))) {
        unsigned int b = a;
        children1.push_back(pi_names[b]);
      } else if (topo_aig.is_constant(topo_aig.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children1.push_back(constant_name);
      } else
        children1.push_back(fanin_name);

      pabc::Io_ReadCreateBuf(abc_ntk_, (char*)(children1[0].c_str()),
                             (char*)(po_names[index].c_str()));
    }
  });

  return abc_ntk_;
}

pabc::Abc_Ntk_t* mockturtle2abc_m(const mockturtle::mig_network& mig) {
  // Convert mig network to the netlist network in abc
  const int MAX = 3;
  bool const0_created = false;
  std::string constant_name = "1\'b0";
  // Initialize the abc network
  abc_ntk_ = pabc::Abc_NtkAlloc(pabc::ABC_NTK_NETLIST, pabc::ABC_FUNC_SOP, 1);
  abc_ntk_->pName = strdup("netlist");

  mockturtle::topo_view topo_mig{mig};

  // create abc network
  // process the abc network primary input
  topo_mig.foreach_pi([&](auto const& n, auto index) {
    const auto index1 = topo_mig.node_to_index(n);
    unsigned int a = index1;
    std::string input_name = pi_names[a];
    pabc::Io_ReadCreatePi(abc_ntk_, (char*)(input_name.c_str()));
  });

  // process the abc network node
  topo_mig.foreach_node([&](auto n) {
    if (topo_mig.is_constant(n) || topo_mig.is_pi(n)) return true;

    auto func = topo_mig.node_function(n);
    std::string node_name = std::to_string(topo_mig.node_to_index(n));

    std::vector<std::string> children;
    children.clear();
    char* pNamesIn[MAX];
    topo_mig.foreach_fanin(n, [&](auto const& f) {
      auto index = topo_mig.node_to_index(topo_mig.get_node(f));
      std::string fanin_name = std::to_string(index);

      if (topo_mig.is_pi(topo_mig.get_node(f))) {
        unsigned int b = index;
        children.push_back(pi_names[b]);
      } else if (topo_mig.is_constant(topo_mig.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children.push_back(constant_name);
      } else
        children.push_back(fanin_name);
    });

    for (size_t i = 0; i < children.size(); i++)
      pNamesIn[i] = (char*)(children[i].c_str());

    pabc::Abc_Obj_t* pNode;
    pNode = pabc::Io_ReadCreateNode(abc_ntk_, (char*)(node_name.c_str()),
                                    pNamesIn, topo_mig.fanin_size(n));
    std::string abc_func = "";
    int count = 0;
    for (auto cube : kitty::isop(func)) {
      topo_mig.foreach_fanin(n, [&](auto const& f, auto index) {
        if (cube.get_mask(index) && topo_mig.is_complemented(f))
          cube.flip_bit(index);
      });

      for (auto i = 0u; i < topo_mig.fanin_size(n); ++i)
        abc_func += (cube.get_mask(i) ? (cube.get_bit(i) ? '1' : '0') : '-');
      abc_func += " 1\n";
      count++;
    }

    pabc::Abc_ObjSetData(
        pNode, pabc::Abc_SopRegister((pabc::Mem_Flex_t*)abc_ntk_->pManFunc,
                                     abc_func.c_str()));
    return true;
  });

  // process the abc network primary output
  std::vector<std::string> children1;
  topo_mig.foreach_po([&](auto const& f, auto index) {
    auto a = topo_mig.node_to_index(topo_mig.get_node(f));
    std::string fanin_name = std::to_string(a);

    pabc::Io_ReadCreatePo(abc_ntk_, (char*)(po_names[index].c_str()));

    if (topo_mig.is_complemented(f)) {
      children1.clear();
      if (topo_mig.is_pi(topo_mig.get_node(f))) {
        unsigned int b = a;
        children1.push_back(pi_names[b]);
      } else if (topo_mig.is_constant(topo_mig.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children1.push_back(constant_name);
      } else
        children1.push_back(fanin_name);
      pabc::Io_ReadCreateInv(abc_ntk_, (char*)(children1[0].c_str()),
                             (char*)(po_names[index].c_str()));
    } else {
      children1.clear();
      if (topo_mig.is_pi(topo_mig.get_node(f))) {
        unsigned int b = a;
        children1.push_back(pi_names[b]);
      } else if (topo_mig.is_constant(topo_mig.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children1.push_back(constant_name);
      } else
        children1.push_back(fanin_name);

      pabc::Io_ReadCreateBuf(abc_ntk_, (char*)(children1[0].c_str()),
                             (char*)(po_names[index].c_str()));
    }
  });

  return abc_ntk_;
}

pabc::Abc_Ntk_t* mockturtle2abc_g(const mockturtle::xag_network& xag) {
  // Convert xag network to the netlist network in abc
  const int MAX = 3;
  bool const0_created = false;
  std::string constant_name = "1\'b0";
  // Initialize the abc network
  abc_ntk_ = pabc::Abc_NtkAlloc(pabc::ABC_NTK_NETLIST, pabc::ABC_FUNC_SOP, 1);
  abc_ntk_->pName = strdup("netlist");

  mockturtle::topo_view topo_xag{xag};

  // create abc network
  // process the abc network primary input
  topo_xag.foreach_pi([&](auto const& n, auto index) {
    const auto index1 = topo_xag.node_to_index(n);
    unsigned int a = index1;
    std::string input_name = pi_names[a];
    pabc::Io_ReadCreatePi(abc_ntk_, (char*)(input_name.c_str()));
  });

  // process the abc network node
  topo_xag.foreach_node([&](auto n) {
    if (topo_xag.is_constant(n) || topo_xag.is_pi(n)) return true;

    auto func = topo_xag.node_function(n);
    std::string node_name = std::to_string(topo_xag.node_to_index(n));

    std::vector<std::string> children;
    children.clear();
    char* pNamesIn[MAX];
    topo_xag.foreach_fanin(n, [&](auto const& f) {
      auto index = topo_xag.node_to_index(topo_xag.get_node(f));
      std::string fanin_name = std::to_string(index);

      if (topo_xag.is_pi(topo_xag.get_node(f))) {
        unsigned int b = index;
        children.push_back(pi_names[b]);
      } else if (topo_xag.is_constant(topo_xag.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children.push_back(constant_name);
      } else
        children.push_back(fanin_name);
    });

    for (size_t i = 0; i < children.size(); i++)
      pNamesIn[i] = (char*)(children[i].c_str());

    pabc::Abc_Obj_t* pNode;
    pNode = pabc::Io_ReadCreateNode(abc_ntk_, (char*)(node_name.c_str()),
                                    pNamesIn, topo_xag.fanin_size(n));
    std::string abc_func = "";
    int count = 0;
    for (auto cube : kitty::isop(func)) {
      topo_xag.foreach_fanin(n, [&](auto const& f, auto index) {
        if (cube.get_mask(index) && topo_xag.is_complemented(f))
          cube.flip_bit(index);
      });

      for (auto i = 0u; i < topo_xag.fanin_size(n); ++i)
        abc_func += (cube.get_mask(i) ? (cube.get_bit(i) ? '1' : '0') : '-');
      abc_func += " 1\n";
      count++;
    }

    pabc::Abc_ObjSetData(
        pNode, pabc::Abc_SopRegister((pabc::Mem_Flex_t*)abc_ntk_->pManFunc,
                                     abc_func.c_str()));
    return true;
  });

  // process the abc network primary output
  std::vector<std::string> children1;
  topo_xag.foreach_po([&](auto const& f, auto index) {
    auto a = topo_xag.node_to_index(topo_xag.get_node(f));
    std::string fanin_name = std::to_string(a);

    pabc::Io_ReadCreatePo(abc_ntk_, (char*)(po_names[index].c_str()));

    if (topo_xag.is_complemented(f)) {
      children1.clear();
      if (topo_xag.is_pi(topo_xag.get_node(f))) {
        unsigned int b = a;
        children1.push_back(pi_names[b]);
      } else if (topo_xag.is_constant(topo_xag.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children1.push_back(constant_name);
      } else
        children1.push_back(fanin_name);
      pabc::Io_ReadCreateInv(abc_ntk_, (char*)(children1[0].c_str()),
                             (char*)(po_names[index].c_str()));
    } else {
      children1.clear();
      if (topo_xag.is_pi(topo_xag.get_node(f))) {
        unsigned int b = a;
        children1.push_back(pi_names[b]);
      } else if (topo_xag.is_constant(topo_xag.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children1.push_back(constant_name);
      } else
        children1.push_back(fanin_name);

      pabc::Io_ReadCreateBuf(abc_ntk_, (char*)(children1[0].c_str()),
                             (char*)(po_names[index].c_str()));
    }
  });

  return abc_ntk_;
}

pabc::Abc_Ntk_t* mockturtle2abc_l(const mockturtle::klut_network& klut) {
  // Convert klut network to the netlist network in abc
  const int MAX = 3;
  bool const0_created = false;
  std::string constant_name = "1\'b0";
  // Initialize the abc network
  abc_ntk_ = pabc::Abc_NtkAlloc(pabc::ABC_NTK_NETLIST, pabc::ABC_FUNC_SOP, 1);
  abc_ntk_->pName = strdup("netlist");

  mockturtle::topo_view topo_klut{klut};

  // create abc network
  // process the abc network primary input
  topo_klut.foreach_pi([&](auto const& n, auto index) {
    const auto index1 = topo_klut.node_to_index(n);
    unsigned int a = index1;
    std::string input_name = pi_names[a];
    pabc::Io_ReadCreatePi(abc_ntk_, (char*)(input_name.c_str()));
  });

  // process the abc network node
  topo_klut.foreach_node([&](auto n) {
    if (topo_klut.is_constant(n) || topo_klut.is_pi(n)) return true;

    auto func = topo_klut.node_function(n);
    std::string node_name = std::to_string(topo_klut.node_to_index(n));

    std::vector<std::string> children;
    children.clear();
    char* pNamesIn[MAX];
    topo_klut.foreach_fanin(n, [&](auto const& f) {
      auto index = topo_klut.node_to_index(topo_klut.get_node(f));
      std::string fanin_name = std::to_string(index);

      if (topo_klut.is_pi(topo_klut.get_node(f))) {
        unsigned int b = index;
        children.push_back(pi_names[b]);
      } else if (topo_klut.is_constant(topo_klut.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children.push_back(constant_name);
      } else
        children.push_back(fanin_name);
    });

    for (size_t i = 0; i < children.size(); i++)
      pNamesIn[i] = (char*)(children[i].c_str());

    pabc::Abc_Obj_t* pNode;
    pNode = pabc::Io_ReadCreateNode(abc_ntk_, (char*)(node_name.c_str()),
                                    pNamesIn, topo_klut.fanin_size(n));
    std::string abc_func = "";
    int count = 0;
    for (auto cube : kitty::isop(func)) {
      topo_klut.foreach_fanin(n, [&](auto const& f, auto index) {
        if (cube.get_mask(index) && topo_klut.is_complemented(f))
          cube.flip_bit(index);
      });

      for (auto i = 0u; i < topo_klut.fanin_size(n); ++i)
        abc_func += (cube.get_mask(i) ? (cube.get_bit(i) ? '1' : '0') : '-');
      abc_func += " 1\n";
      count++;
    }

    pabc::Abc_ObjSetData(
        pNode, pabc::Abc_SopRegister((pabc::Mem_Flex_t*)abc_ntk_->pManFunc,
                                     abc_func.c_str()));
    return true;
  });

  // process the abc network primary output
  std::vector<std::string> children1;
  topo_klut.foreach_po([&](auto const& f, auto index) {
    auto a = topo_klut.node_to_index(topo_klut.get_node(f));
    std::string fanin_name = std::to_string(a);

    pabc::Io_ReadCreatePo(abc_ntk_, (char*)(po_names[index].c_str()));

    if (topo_klut.is_complemented(f)) {
      children1.clear();
      if (topo_klut.is_pi(topo_klut.get_node(f))) {
        unsigned int b = a;
        children1.push_back(pi_names[b]);
      } else if (topo_klut.is_constant(topo_klut.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children1.push_back(constant_name);
      } else
        children1.push_back(fanin_name);
      pabc::Io_ReadCreateInv(abc_ntk_, (char*)(children1[0].c_str()),
                             (char*)(po_names[index].c_str()));
    } else {
      children1.clear();
      if (topo_klut.is_pi(topo_klut.get_node(f))) {
        unsigned int b = a;
        children1.push_back(pi_names[b]);
      } else if (topo_klut.is_constant(topo_klut.get_node(f))) {
        // process the abc network constant node
        if (!const0_created) {
          pabc::Abc_NtkFindOrCreateNet(abc_ntk_,
                                       (char*)(constant_name.c_str()));
          pabc::Io_ReadCreateConst(abc_ntk_, (char*)(constant_name.c_str()), 0);
          const0_created = true;
        }
        children1.push_back(constant_name);
      } else
        children1.push_back(fanin_name);

      pabc::Io_ReadCreateBuf(abc_ntk_, (char*)(children1[0].c_str()),
                             (char*)(po_names[index].c_str()));
    }
  });

  return abc_ntk_;
}
}  // namespace phyLS

#endif
