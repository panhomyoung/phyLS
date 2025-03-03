#pragma once

#include <fmt/format.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <mockturtle/algorithms/mapper.hpp>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "assert.h"
#include "../core/MCTS.hpp"
#include "assert.h"
#include "base/abc/abc.h"
#include "base/main/mainFrame.c"
#include "map/scl/scl.c"
#include "map/scl/sclSize.c"

using namespace pabc;

namespace phyLS {

void read_mcts_ps(std::string file_path, MCTS::MCTS_params &ps) {
  std::ifstream ifs(file_path, std::ifstream::in);
  assert(ifs.is_open());
}

void read_def_file(std::string file_path,
                   std::vector<node_position> &Vec_position, int input_size = 0) {
  std::ifstream ifs(file_path, std::ifstream::in);
  assert(ifs.is_open());
  std::string line;
  int cell_size = 0;
  bool gift = false;
  while (std::getline(ifs, line)) {
    if (line.substr(0, 4) == "COMP") {
      while (std::getline(ifs, line)) {
        if (line.substr(0, 3) == "END") {
          break;
        } else {
          std::string substr;
          // std::cout<<line<<"\n";
          std::string::size_type found_s = line.find_first_of("-");
          if (found_s != std::string::npos) {
            substr = line.substr(found_s);
          } else {
            continue;
          }
          if (substr[2] == 'g') {
            int index;
            std::string res;
            std::string::size_type found_b = line.find_first_of("g");
            std::string::size_type found_e = line.find_first_of("A");
            while (found_b != found_e - 1) {
              res += line[found_b + 1];
              found_b++;
            }
            index = std::stoi(res);
            cell_size++;

            // read next line
            while (line.find("PLACED") == std::string::npos)
              std::getline(ifs, line);
            // get position of AIG node
            assert(line.find("PLACED") != std::string::npos);
            std::string position;
            std::string::size_type found_1 = line.find_first_of("(");
            std::string::size_type found_2 = line.find_first_of(")");
            while (found_1 != found_2 - 1) {
              position += line[found_1 + 1];
              found_1++;
            }
            std::regex pattern(R"((\d+)\s+(\d+))");
            std::smatch match;
            std::regex_search(position, match, pattern);
            if (!match.empty()) {
              // std::cout << match[1] << " " << match[2] << "\n";
              Vec_position[index + input_size + 1].x_coordinate = std::stoi(match[1]);
              Vec_position[index + input_size + 1].y_coordinate = std::stoi(match[2]);
            }
            else {
              std::cerr << "Error at : " << line <<"\n";
            }
          }
          else if (substr.substr(2, 3) == "and") {
            // get index of AIG node
            int index;
            gift = true;
            std::string res;
            std::string::size_type found_b = line.find_first_of("_");
            std::string::size_type found_e = line.find_last_of("_");
            while (found_b != found_e - 1) {
              res += line[found_b + 1];
              found_b++;
            }
            index = std::stoi(res);
            cell_size = index;
            // read next line
            std::getline(ifs, line);
            // get position of AIG node
            bool is_x = true;
            std::string position = "";
            for (char c : line) {
              if (std::isdigit(c)) {
                position += c;
              } else if (!position.empty()) {
                if (is_x) {
                  Vec_position[index].x_coordinate = std::stoi(position);
                  position.clear();
                  is_x = false;
                } else {
                  Vec_position[index].y_coordinate = std::stoi(position);
                  position.clear();
                }
              } else {
                continue;
              }
            }
          }
          else {
            continue;
          }
        }
      }
    } else if (line.substr(0, 4) == "PINS") {
      while (std::getline(ifs, line)) {
        if (line.substr(0, 3) == "END") {
          break;
        } else {
          std::string::size_type fidx = line.find_first_not_of(" ");
          if (line[fidx] != '-')
          {
            // std::cerr << "Error at : " << line <<"\n";
            continue;
          }
          std::string et_line = line;
          std::getline(ifs, line);
          et_line += line;
          // get input position
          if (et_line.find("INPUT") != std::string::npos) 
          {
            std::regex pattern(R"((\d+))");
            std::smatch match;
            std::regex_search(et_line, match, pattern);
            int index = 0;
            if (!match.empty()) {
              index = std::stoi(match[1]);
            }
            else {
              std::cout<<"error\n";
              continue;
            }
            
            while (line.find("PLACED") == std::string::npos) 
              std::getline(ifs, line);

            std::string position = "";
            std::string::size_type found_1 = line.find_first_of("(");
            std::string::size_type found_2 = line.find_first_of(")");
            while (found_1 != found_2 - 1) {
              position += line[found_1 + 1];
              found_1++;
            }
            std::regex pattern2(R"((\d+)\s+(\d+))");
            std::smatch match2;
            std::regex_search(position, match2, pattern2);
            // std::cout<<"input\n";
            Vec_position[index + 1].x_coordinate = std::stoi(match2[1]);
            Vec_position[index + 1].y_coordinate = std::stoi(match2[2]);
            continue;
          }
          else if (et_line.find("OUTPUT") != std::string::npos)
          {
            std::regex pattern(R"((\d+))");
            std::smatch match;
            std::regex_search(et_line, match, pattern);
            int index = 0;
            if (!match.empty()) {
              index = std::stoi(match[1]);
            }
            else {
              std::cout<<"error\n";
              continue;
            }
            
            while (line.find("PLACED") == std::string::npos) 
              std::getline(ifs, line);
            
            std::string position = "";
            std::string::size_type found_1 = line.find_first_of("(");
            std::string::size_type found_2 = line.find_first_of(")");
            while (found_1 != found_2 - 1) {
              position += line[found_1 + 1];
              found_1++;
            }
            std::regex pattern2(R"((\d+)\s+(\d+))");
            std::smatch match2;
            std::regex_search(position, match2, pattern2);
            int sa;
            if (gift) 
            {
              sa = index + cell_size + 1;
            } else {
              sa = index + cell_size + input_size + 1;
            }
            Vec_position[sa].x_coordinate = std::stoi(match2[1]);
            Vec_position[sa].y_coordinate = std::stoi(match2[2]);
            continue;
          }
        }
      }
    } else {
      continue;
    }
  }
}

void read_def_file_openroad(
    std::string file_path, std::vector<mockturtle::node_position> &Vec_position,
    int input_size) {
  std::cout << "reading def file from " << file_path << "\n";
  std::ifstream ifs(file_path, std::ifstream::in);
  assert(ifs.is_open());
  std::string line;
  int cell_size = 0;
  while (std::getline(ifs, line)) {
    if (line.substr(0, 10) == "COMPONENTS") {
      while (std::getline(ifs, line)) {
        if (line.substr(0, 3) == "END") {
          break;
        } else {
          if (line.find("PLACED") != std::string::npos) {
            // get index of AIG node
            int index;
            std::string res;
            std::string::size_type found_b = line.find_first_of("g");
            std::string::size_type found_e = line.find_first_of("A");
            while (found_b != found_e - 1) {
              res += line[found_b + 1];
              found_b++;
            }
            index = std::stoi(res);
            cell_size = index;
            std::string position;
            std::string::size_type found_1 = line.find_first_of("(");
            std::string::size_type found_2 = line.find_first_of(")");
            while (found_1 != found_2 - 1) {
              position += line[found_1 + 1];
              found_1++;
            }
            std::regex pattern(R"((\d+)\s+(\d+))");
            std::smatch match;
            std::regex_search(position, match, pattern);
            Vec_position[index + input_size + 1].x_coordinate =
                std::stoi(match[1]);
            Vec_position[index + input_size + 1].y_coordinate =
                std::stoi(match[2]);
          } else {
            continue;
          }
        }
      }
    } else if (line.substr(0, 4) == "PINS") {
      while (std::getline(ifs, line)) {
        if (line.substr(0, 3) == "END") {
          break;
        } else {
          if (line.find("INPUT") != std::string::npos) {
            int index;
            std::string res;
            std::string::size_type found_b = line.find_first_of("x");
            std::string::size_type found_e = line.find_first_of("+");
            while (found_b != found_e - 1) {
              res += line[found_b + 1];
              found_b++;
            }
            index = std::stoi(res);
            std::getline(ifs, line);
            std::getline(ifs, line);
            std::getline(ifs, line);
            std::string position;
            std::string::size_type found_1 = line.find_first_of("(");
            std::string::size_type found_2 = line.find_first_of(")");
            while (found_1 != found_2 - 1) {
              position += line[found_1 + 1];
              found_1++;
            }
            std::regex pattern(R"((\d+)\s+(\d+))");
            std::smatch match;
            std::regex_search(position, match, pattern);
            Vec_position[index + 1].x_coordinate = std::stoi(match[1]);
            Vec_position[index + 1].y_coordinate = std::stoi(match[2]);
          } else if (line.find("OUTPUT") != std::string::npos) {
            int index;
            std::string res;
            std::string::size_type found_b = line.find_first_of("y");
            std::string::size_type found_e = line.find_first_of("+");
            while (found_b != found_e - 1) {
              res += line[found_b + 1];
              found_b++;
            }
            index = std::stoi(res);
            std::getline(ifs, line);
            std::getline(ifs, line);
            std::getline(ifs, line);
            std::string position;
            std::string::size_type found_1 = line.find_first_of("(");
            std::string::size_type found_2 = line.find_first_of(")");
            while (found_1 != found_2 - 1) {
              position += line[found_1 + 1];
              found_1++;
            }
            std::regex pattern(R"((\d+)\s+(\d+))");
            std::smatch match;
            std::regex_search(position, match, pattern);
            Vec_position[index + cell_size + input_size + 2].x_coordinate =
                std::stoi(match[1]);
            Vec_position[index + cell_size + input_size + 2].y_coordinate =
                std::stoi(match[2]);
          } else {
            continue;
          }
        }
      }
    } else {
      continue;
    }
  }
}

// void read_def_file(std::string file_path,
//                    std::vector<mockturtle::node_position> &Vec_position,
//                    int input_size = 0) {
//   std::ifstream ifs(file_path, std::ifstream::in);
//   assert(ifs.is_open());
//   std::string line;
//   while (std::getline(ifs, line)) {
//     if (line.substr(0, 4) == "COMP") {
//       while (std::getline(ifs, line)) {
//         if (line.substr(0, 3) == "END") {
//           break;
//         } else {
//           // get index of AIG node
//           int index;
//           std::string res;
//           std::string::size_type found_b = line.find_first_of("_");
//           std::string::size_type found_e = line.find_last_of("_");
//           while (found_b != found_e - 1) {
//             res += line[found_b + 1];
//             found_b++;
//           }
//           index = std::stoi(res);
//           // read next line
//           std::getline(ifs, line);
//           // get position of AIG node
//           bool is_x = true;
//           std::string position = "";
//           for (char c : line) {
//             if (std::isdigit(c)) {
//               position += c;
//             } else if (!position.empty()) {
//               if (is_x) {
//                 Vec_position[index].x_coordinate = std::stoi(position);
//                 position.clear();
//                 is_x = false;
//               } else {
//                 Vec_position[index].y_coordinate = std::stoi(position);
//                 position.clear();
//               }
//             } else {
//               continue;
//             }
//           }
//         }
//       }
//     } else if (line.substr(0, 4) == "PINS") {
//       while (std::getline(ifs, line)) {
//         if (line.substr(0, 3) == "END") {
//           break;
//         } else if (line.find("input") != std::string::npos &&
//                    line.find("clk") == std::string::npos) {
//           // get input position
//           std::string::size_type found_1 = line.find("_");
//           std::string::size_type found_2 = line.find("+");
//           int index;
//           std::string res;
//           while (found_1 != found_2 - 2) {
//             res += line[found_1 + 1];
//             found_1++;
//           }
//           index = std::stoi(res);
//           std::getline(ifs, line);
//           std::getline(ifs, line);
//           bool is_x = true;
//           std::string position_input = "";
//           for (char c : line) {
//             if (std::isdigit(c)) {
//               position_input += c;
//             } else if (!position_input.empty()) {
//               if (is_x) {
//                 Vec_position[index + 1].x_coordinate =
//                     std::stoi(position_input);
//                 position_input.clear();
//                 is_x = false;
//               } else {
//                 Vec_position[index + 1].y_coordinate =
//                     std::stoi(position_input);
//                 position_input.clear();
//               }
//             } else {
//               continue;
//             }
//           }
//         } else {
//           continue;
//         }
//       }
//     } else {
//       continue;
//     }
//   }
// }

void read_pl_file(std::string file_path,
                  std::vector<mockturtle::node_position> &Vec_position,
                  uint32_t ntk_size) {
  std::ifstream ifs(file_path, std::ifstream::in);
  assert(ifs.is_open());
  std::string line;
  uint32_t index;
  Vec_position.reserve(ntk_size);
  while (getline(ifs, line)) {
    if (line[0] == 'U') {
      continue;
    } else {
      for (char c : line) {
        std::string str_number = "";
        uint32_t a = 0;
        if (c == ':') {
          break;
        }
        if (std::isdigit(c)) {
          str_number += c;
        } else {
          if (!str_number.empty()) {
            if (a == 0) {
              index = std::stoi(str_number);
              str_number = "";
              a++;
            } else if (a == 1) {
              Vec_position[index].x_coordinate = std::stoi(str_number);
              str_number = "";
              a++;
            } else if (a == 2) {
              Vec_position[index].y_coordinate = std::stoi(str_number);
              str_number = "";
              a = 0;
            }
          }
        }
      }
    }
  }
  ifs.close();
}

void read_def_file_abc(std::string file_path, Vec_Int_t **VecNP) {
  std::ifstream ifs(file_path, std::ifstream::in);
  assert(ifs.is_open());
  std::string line;
  int cell_size = 0;
  while (std::getline(ifs, line)) {
    if (line.substr(0, 4) == "COMP") {
      while (std::getline(ifs, line)) {
        if (line.substr(0, 3) == "END") {
          break;
        } else {
          // get index of AIG node
          int index;
          std::string res;
          std::string::size_type found_b = line.find_first_of("_");
          std::string::size_type found_e = line.find_last_of("_");
          while (found_b != found_e - 1) {
            res += line[found_b + 1];
            found_b++;
          }
          index = std::stoi(res);
          cell_size = index;
          // read next line
          std::getline(ifs, line);
          // get position of AIG node
          bool is_x = true;
          std::string position = "";
          for (char c : line) {
            if (std::isdigit(c)) {
              position += c;
            } else if (!position.empty()) {
              if (is_x) {
                pabc::Vec_IntPush(VecNP[index - 1], std::stoi(position));
                position.clear();
                is_x = false;
              } else {
                pabc::Vec_IntPush(VecNP[index - 1], std::stoi(position));
                position.clear();
              }
            } else {
              continue;
            }
          }
        }
      }
    } else if (line.substr(0, 4) == "PINS") {
      while (std::getline(ifs, line)) {
        if (line.substr(0, 3) == "END") {
          break;
        } else if (line.find("input") != std::string::npos &&
                   line.find("clk") == std::string::npos) {
          // get input position
          std::string::size_type found_1 = line.find("_");
          std::string::size_type found_2 = line.find("+");
          int index;
          std::string res;
          while (found_1 != found_2 - 2) {
            res += line[found_1 + 1];
            found_1++;
          }
          index = std::stoi(res);
          std::getline(ifs, line);
          std::getline(ifs, line);
          bool is_x = true;
          std::string position_input = "";
          for (char c : line) {
            if (std::isdigit(c)) {
              position_input += c;
            } else if (!position_input.empty()) {
              if (is_x) {
                pabc::Vec_IntPush(VecNP[index], std::stoi(position_input));
                position_input.clear();
                is_x = false;
              } else {
                pabc::Vec_IntPush(VecNP[index], std::stoi(position_input));
                position_input.clear();
              }
            } else {
              continue;
            }
          }
        } else if (line.find("output") != std::string::npos) {
          // get input position
          std::string::size_type found_1 = line.find("_");
          std::string::size_type found_2 = line.find("+");
          int index;
          std::string res;
          while (found_1 != found_2 - 2) {
            res += line[found_1 + 1];
            found_1++;
          }
          index = std::stoi(res);
          std::getline(ifs, line);
          std::getline(ifs, line);
          bool is_x = true;
          std::string position_output = "";
          for (char c : line) {
            if (std::isdigit(c)) {
              position_output += c;
            } else if (!position_output.empty()) {
              if (is_x) {
                pabc::Vec_IntPush(VecNP[index + cell_size],
                                  std::stoi(position_output));
                position_output.clear();
                is_x = false;
              } else {
                pabc::Vec_IntPush(VecNP[index + cell_size],
                                  std::stoi(position_output));
                position_output.clear();
              }
            } else {
              continue;
            }
          }
        } else {
          continue;
        }
      }
    } else {
      continue;
    }
  }
}

void npTrans(std::vector<mockturtle::node_position> np, Vec_Int_t **VecNP) {
  for (int i = 0; i < np.size(); i++) {
    pabc::Vec_IntPush(VecNP[i], np[i].x_coordinate);
    pabc::Vec_IntPush(VecNP[i], np[i].y_coordinate);
  }
}

std::pair<double, double> stime(std::string lib_file,
                                std::string netlist_file) {
  Abc_Ntk_t *pNtk;
  pabc::Abc_Frame_t *pAbc = pabc::Abc_FrameGetGlobalFrame();
  SC_DontUse dont_use = {0};
  SC_Lib *pLib =
      Scl_ReadLibraryFile(pAbc, (char *)(lib_file.c_str()), 1, 0, dont_use);
  ABC_FREE(dont_use.dont_use_list);
  Abc_SclLoad(pLib, (SC_Lib **)&pAbc->pLibScl);
  if (pAbc->pLibScl) {
    Abc_SclInstallGenlib(pAbc->pLibScl, 0, 0, 0);
    Mio_LibraryTransferCellIds();
  }

  if (Abc_FrameReadLibGen() == NULL) {
    Abc_Print(-1, "Cannot read mapped design when the library is not given.\n");
  }
  for (auto &x : netlist_file) {
    if (x == '>' || x == '\\') x = '/';
  }
  pNtk = Io_Read((char *)(netlist_file.c_str()), IO_FILE_VERILOG, 1, 0);

  // set defaults
  int fUseWireLoads = 1;
  int fPrintPath = 0;

  double area;
  double delay;
  Abc_SclTimePerformdelay(pLib, pNtk, 0, fUseWireLoads, 0, fPrintPath, 0, delay,
                          area);
  return std::make_pair(delay, area);
}

std::pair<double, double> _stime(std::string lib_file, std::string netlist_file,
                                 std::string def_file) {
  Abc_Ntk_t *pNtk;
  pabc::Abc_Frame_t *pAbc = pabc::Abc_FrameGetGlobalFrame();
  SC_DontUse dont_use = {0};
  SC_Lib *pLib =
      Scl_ReadLibraryFile(pAbc, (char *)(lib_file.c_str()), 1, 0, dont_use);
  ABC_FREE(dont_use.dont_use_list);
  Abc_SclLoad(pLib, (SC_Lib **)&pAbc->pLibScl);
  if (pAbc->pLibScl) {
    Abc_SclInstallGenlib(pAbc->pLibScl, 0, 0, 0);
    Mio_LibraryTransferCellIds();
  }

  if (Abc_FrameReadLibGen() == NULL) {
    Abc_Print(-1, "Cannot read mapped design when the library is not given.\n");
  }
  for (auto &x : netlist_file) {
    if (x == '>' || x == '\\') x = '/';
  }
  pNtk = Io_Read((char *)(netlist_file.c_str()), IO_FILE_VERILOG, 1, 0);
  int np_size = Abc_NtkNodeNum(pNtk) + Abc_NtkPiNum(pNtk) + Abc_NtkPoNum(pNtk);

  std::vector<mockturtle::node_position> np(np_size + 1);
  read_def_file(def_file, np, Abc_NtkPiNum(pNtk));
  np.erase(np.begin());

  // set defaults
  int fUseWireLoads = 1;
  int fPrintPath = 0;

  Vec_Int_t **VecNP;  // vector<vector<int>>
  // std::cout<<"size of given np = "<<np.size()<<"\t";
  // std::cout<<"the NP vector should be in size of "<<np_size<<"\n";
  VecNP = ABC_ALLOC(Vec_Int_t *, np_size);
  if (VecNP == NULL) {
    std::cerr << "Memory allocation for VecNP failed!" << std::endl;
  }

  // std::cout << "flag1\n";
  for (int i = 0; i < np_size; i++) {
    VecNP[i] = Vec_IntAllocExact(2);
  }

  // std::cout<<"flag1.5\n";
  phyLS::npTrans(np, VecNP);
  pNtk->vPlace = VecNP;
  // std::cout<<"flag2\n";
  // for (int j = 0; j < np_size; j++)
  // {
  //   std::cout<<"vec size is "<<Vec_IntSize(VecNP[j])<<"\t";
  //   std::cout<<"position of gate "<<j<<" : ";
  //   for (int i = 0; i < Vec_IntSize(VecNP[j]); i++)
  //     std::cout <<Vec_IntEntry(VecNP[j], i) << " ";
  //   std::cout << std::endl;
  // }
  // std::cout<<"flag3\n";
  double area;
  double delay;
  Abc_SclTimePerformdelayNew(pLib, pNtk, 0, fUseWireLoads, 0, fPrintPath, 0,
                             delay, area);
  return std::make_pair(delay, area);
}

void stime_of_res(std::string lib_file, std::string netlist_file,
                  std::vector<mockturtle::node_position> np, double &maxDelay,
                  double &area) {
  /* compute baseline */
  // auto pair = _stime(lib_file,
  // "/home/panhongxiang/OpenROAD-flow-scripts/flow/experimental_result/baseline/adder/results/base/1_synth.v",
  //       "/home/panhongxiang/OpenROAD-flow-scripts/flow/experimental_result/baseline/adder/results/base/3_3_place_gp.def");
  // std::cout<<"baseline:   Delay reward : "<<pair.first<<". Area reward :
  // "<<pair.second<<" Reward : "<<(pair.first * pair.second)<<"\n";

  Abc_Ntk_t *pNtk;
  pabc::Abc_Frame_t *pAbc = pabc::Abc_FrameGetGlobalFrame();
  SC_DontUse dont_use = {0};
  SC_Lib *pLib =
      Scl_ReadLibraryFile(pAbc, (char *)(lib_file.c_str()), 1, 0, dont_use);
  ABC_FREE(dont_use.dont_use_list);
  Abc_SclLoad(pLib, (SC_Lib **)&pAbc->pLibScl);
  if (pAbc->pLibScl) {
    Abc_SclInstallGenlib(pAbc->pLibScl, 0, 0, 0);
    Mio_LibraryTransferCellIds();
  }

  if (Abc_FrameReadLibGen() == NULL) {
    Abc_Print(-1, "Cannot read mapped design when the library is not given.\n");
  }
  for (auto &x : netlist_file) {
    if (x == '>' || x == '\\') x = '/';
  }
  std::cout << "reading netlist : " << netlist_file << "\n";
  pNtk = Io_Read((char *)(netlist_file.c_str()), IO_FILE_VERILOG, 1, 0);
  std::cout << "STA intermediate netlist from \"" << netlist_file << "\"\n";
  int np_size = Abc_NtkNodeNum(pNtk) + Abc_NtkPiNum(pNtk) + Abc_NtkPoNum(pNtk);

  std::cout << "needed size of np : " << np_size;
  std::cout << ", given size of np : " << np.size() << "\n";

  if (np_size != np.size()) {
    std::cerr << "\"size of netlist position don't fit!\"\n";
    maxDelay = area = std::numeric_limits<double>::max() * 0.001;
    return;
  }

  // set defaults
  int fUseWireLoads = 1;
  int fPrintPath = 0;

  // std::vector<int> constant;
  // if (np_size != np.size()) {
  //   pNtk = Abc_NtkDupDfs(pNtk);
  //   Abc_FrameReplaceCurrentNetwork(pAbc, pNtk);
  //   std::cout << "size: "
  //             << Abc_NtkNodeNum(pNtk) + Abc_NtkPiNum(pNtk) +
  //             Abc_NtkPoNum(pNtk)
  //             << std::endl;
  // }

  Abc_Obj_t *pObj;
  int p;
  // Abc_NtkForEachNode(pNtk, pObj, p) {
  //   if (!Abc_ObjFaninNum(pObj)) {
  //     std::cout << "pObj " << pObj->Id << "   ";
  //     constant.push_back(pObj->Id - Abc_NtkPoNum(pNtk) - 1);
  //   }
  // }
  // std::cout << std::endl;

  Vec_Int_t **VecNP;  // vector<vector<int>>
  VecNP = ABC_ALLOC(Vec_Int_t *, np_size);
  for (int i = 0; i < np_size; i++) VecNP[i] = Vec_IntAlloc(2);
  phyLS::npTrans(np, VecNP);

  // if (constant.size()) {
  //   sort(constant.begin(), constant.end());
  //   for (auto x : constant) {
  //     int k;
  //     Vec_IntPush(VecNP[np_size - 1], 0);
  //     Vec_IntPush(VecNP[np_size - 1], 0);
  //     for (k = np_size - 1; k > x; k--) {
  //       int new0 = VecNP[k - 1]->pArray[0];
  //       int new1 = VecNP[k - 1]->pArray[1];
  //       Vec_IntWriteEntry(VecNP[k], 0, new0);
  //       Vec_IntWriteEntry(VecNP[k], 1, new1);
  //     }
  //     // std::cout << "k=" << k << std::endl;
  //     Vec_IntWriteEntry(VecNP[k], 0, 0);
  //     Vec_IntWriteEntry(VecNP[k], 1, 0);
  //   }
  // }

  pNtk->vPlace = VecNP;

  Abc_SclTimePerformdelayNew(pLib, pNtk, 0, fUseWireLoads, 0, fPrintPath, 0,
                             maxDelay, area);
}


void read_deffile(std::string file_path,
                   std::vector<node_position> &Vec_position,
                   int input_size = 0) {
  std::ifstream ifs(file_path, std::ifstream::in);
  assert(ifs.is_open());
  std::string line;
  int cell_size = 0;
  bool gift = false;
  while (std::getline(ifs, line)) {
    if (line.substr(0, 4) == "COMP") {
      while (std::getline(ifs, line)) {
        if (line.substr(0, 3) == "END") {
          break;
        } else {
          std::string substr;
          // std::cout<<line<<"\n";
          std::string::size_type found_s = line.find_first_of("-");
          if (found_s != std::string::npos) {
            substr = line.substr(found_s);
          } else {
            continue;
          }
          if (substr[2] == 'g') {
            int index;
            std::string res;
            std::string::size_type found_b = line.find_first_of("g");
            std::string::size_type found_e = line.find_first_of("A");
            while (found_b != found_e - 1) {
              res += line[found_b + 1];
              found_b++;
            }
            index = std::stoi(res);
            cell_size++;

            // read next line
            while (line.find("PLACED") == std::string::npos)
              std::getline(ifs, line);
            // get position of AIG node
            assert(line.find("PLACED") != std::string::npos);
            std::string position;
            std::string::size_type found_1 = line.find_first_of("(");
            std::string::size_type found_2 = line.find_first_of(")");
            while (found_1 != found_2 - 1) {
              position += line[found_1 + 1];
              found_1++;
            }
            std::regex pattern(R"((\d+)\s+(\d+))");
            std::smatch match;
            std::regex_search(position, match, pattern);
            if (!match.empty()) {
              // std::cout << match[1] << " " << match[2] << "\n";
              Vec_position[index + input_size + 1].x_coordinate =
                  std::stoi(match[1]);
              Vec_position[index + input_size + 1].y_coordinate =
                  std::stoi(match[2]);
            } else {
              std::cerr << "Error at : " << line << "\n";
            }
          } else if (substr.substr(2, 3) == "and") {
            // get index of AIG node
            int index;
            gift = true;
            std::string res;
            std::string::size_type found_b = line.find_first_of("_");
            std::string::size_type found_e = line.find_last_of("_");
            while (found_b != found_e - 1) {
              res += line[found_b + 1];
              found_b++;
            }
            index = std::stoi(res);
            cell_size = index;
            // read next line
            std::getline(ifs, line);
            // get position of AIG node
            bool is_x = true;
            std::string position = "";
            for (char c : line) {
              if (std::isdigit(c)) {
                position += c;
              } else if (!position.empty()) {
                if (is_x) {
                  Vec_position[index].x_coordinate = std::stoi(position);
                  position.clear();
                  is_x = false;
                } else {
                  Vec_position[index].y_coordinate = std::stoi(position);
                  position.clear();
                }
              } else {
                continue;
              }
            }
          } else {
            continue;
          }
        }
      }
    } else if (line.substr(0, 4) == "PINS") {
      while (std::getline(ifs, line)) {
        if (line.substr(0, 3) == "END") {
          break;
        } else {
          std::string::size_type fidx = line.find_first_not_of(" ");
          if (line[fidx] != '-') {
            // std::cerr << "Error at : " << line <<"\n";
            continue;
          }
          std::string et_line = line;
          std::getline(ifs, line);
          et_line += line;
          // get input position
          if (et_line.find("INPUT") != std::string::npos) {
            std::regex pattern(R"((\d+))");
            std::smatch match;
            std::regex_search(et_line, match, pattern);
            int index = 0;
            if (!match.empty()) {
              index = std::stoi(match[1]);
            } else {
              std::cout << "error\n";
              continue;
            }

            while (line.find("PLACED") == std::string::npos)
              std::getline(ifs, line);

            std::string position = "";
            std::string::size_type found_1 = line.find_first_of("(");
            std::string::size_type found_2 = line.find_first_of(")");
            while (found_1 != found_2 - 1) {
              position += line[found_1 + 1];
              found_1++;
            }
            std::regex pattern2(R"((\d+)\s+(\d+))");
            std::smatch match2;
            std::regex_search(position, match2, pattern2);
            // std::cout<<"input\n";
            Vec_position[index + 1].x_coordinate = std::stoi(match2[1]);
            Vec_position[index + 1].y_coordinate = std::stoi(match2[2]);
            continue;
          } else if (et_line.find("OUTPUT") != std::string::npos) {
            std::regex pattern(R"((\d+))");
            std::smatch match;
            std::regex_search(et_line, match, pattern);
            int index = 0;
            if (!match.empty()) {
              index = std::stoi(match[1]);
            } else {
              std::cout << "error\n";
              continue;
            }

            while (line.find("PLACED") == std::string::npos)
              std::getline(ifs, line);

            std::string position = "";
            std::string::size_type found_1 = line.find_first_of("(");
            std::string::size_type found_2 = line.find_first_of(")");
            while (found_1 != found_2 - 1) {
              position += line[found_1 + 1];
              found_1++;
            }
            std::regex pattern2(R"((\d+)\s+(\d+))");
            std::smatch match2;
            std::regex_search(position, match2, pattern2);
            int sa;
            if (gift) {
              sa = index + cell_size + 1;
            } else {
              sa = index + cell_size + input_size + 1;
            }
            Vec_position[sa].x_coordinate = std::stoi(match2[1]);
            Vec_position[sa].y_coordinate = std::stoi(match2[2]);
            continue;
          }
        }
      }
    } else {
      continue;
    }
  }
}

}  // NAMESPACE phyLS
