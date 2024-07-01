#pragma once

#include <fmt/format.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <mockturtle/algorithms/mapper.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "assert.h"

namespace phyLS {

void read_def_file(std::string file_path,
                   std::vector<mockturtle::node_position> &Vec_position) {
  std::ifstream ifs(file_path, std::ifstream::in);
  assert(ifs.is_open());
  std::string line;
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
                Vec_position[index + 1].x_coordinate =
                    std::stoi(position_input);
                position_input.clear();
                is_x = false;
              } else {
                Vec_position[index + 1].y_coordinate =
                    std::stoi(position_input);
                position_input.clear();
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

}  // NAMESPACE phyLS
